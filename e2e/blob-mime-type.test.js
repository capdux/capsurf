// e2e/blob-mime-type.test.js
// Migrated from e2e/blob-mime-type.e2e.test.ts
// Verifies WHATWG File API normalization on Blob and File types,
// parameter validation, frozen options, and Request/Response MIME passthrough.

const { expect, test } = require("./harness");

// Test 1: 验证 Blob 与 File 对各组 MIME 输入的归一化规则、冻结选项、以及基本类型参数抛出 TypeError
test("Blob and File types use File API normalization", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const inputs = [
        "TEXT/HTML;CHARSET=GBK",
        "text/html;charset=gbk;charset=windows-1255",
        "text/html;charset=gbk(",
        "text/html;charset=\\u000bgbk",
        "a",
        " image/gif ",
        "text/\\u00e5",
      ];
      const errorName = function (construct) {
        try {
          construct();
          return null;
        } catch (error) {
          return error && error.name;
        }
      };
      const captureType = function (construct) {
        try {
          return { value: construct(), error: null };
        } catch (error) {
          return { value: null, error: error && error.name };
        }
      };
      const f = new File(["test data payload"], "sample.txt", { type: "TEXT/PLAIN" });
      const b = new Blob(["hello world"], { type: "TEXT/HTML" });
      const sliced = b.slice(0, 5, "TEXT/PLAIN");

      return {
        types: inputs.map(function (type) {
          return [
            new Blob([], { type: type }).type,
            new File([], "mime-test", { type: type }).type,
          ];
        }),
        frozenTypes: [
          captureType(function () { return new Blob([], Object.freeze({ type: " TEXT/HTML " })).type; }),
          captureType(function () { return new File([], "mime-test", Object.freeze({ type: " TEXT/HTML " })).type; }),
        ],
        primitiveOptions: [
          errorName(function () { return Reflect.construct(Blob, [[], 1]); }),
          errorName(function () { return Reflect.construct(File, [[], "mime-test", 1]); }),
        ],
        fileProperties: {
          name: f.name,
          size: f.size,
          type: f.type,
          isFile: f instanceof File,
          isBlob: f instanceof Blob,
          hasLastModified: typeof f.lastModified === "number",
        },
        blobSlice: {
          originalSize: b.size,
          originalType: b.type,
          slicedSize: sliced.size,
          slicedType: sliced.type,
        },
      };
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const actual = JSON.parse(String(r.result?.value));

  // MIME 归一化规范断言
  expect(actual.types).toEqual([
    ["text/html;charset=gbk", "text/html;charset=gbk"],
    ["text/html;charset=gbk;charset=windows-1255", "text/html;charset=gbk;charset=windows-1255"],
    ["text/html;charset=gbk(", "text/html;charset=gbk("],
    ["", ""],
    ["a", "a"],
    [" image/gif ", " image/gif "],
    ["", ""],
  ]);

  // 冻结对象选项断言
  expect(actual.frozenTypes).toEqual([
    { value: " text/html ", error: null },
    { value: " text/html ", error: null },
  ]);

  // 基本类型参数抛出 TypeError 断言
  expect(actual.primitiveOptions).toEqual(["TypeError", "TypeError"]);

  // File 基础属性断言
  expect(actual.fileProperties.name).toBe("sample.txt");
  expect(actual.fileProperties.size).toBe(17);
  expect(actual.fileProperties.type).toBe("text/plain");
  expect(actual.fileProperties.isFile).toBe(true);
  expect(actual.fileProperties.isBlob).toBe(true);
  expect(actual.fileProperties.hasLastModified).toBe(true);

  // Blob slice 断言
  expect(actual.blobSlice.originalSize).toBe(11);
  expect(actual.blobSlice.originalType).toBe("text/html");
  expect(actual.blobSlice.slicedSize).toBe(5);
  expect(actual.blobSlice.slicedType).toBe("text/plain");
});

// Test 2: 验证 Request 和 Response 的 .blob() 透传并保留 Content-Type 头部的大小写敏感 MIME 参数
test("Request and Response blobs retain MIME serialization", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const headers = { "content-type": "TEXT/HTML;CHARSET=GBK" };
      const request = new Request("https://example.test/", {
        method: "POST",
        headers: headers,
        body: "request body",
      });
      const response = new Response("response body", { headers: headers });
      const shadowedResponse = new Response("shadowed body", { headers: headers });
      Object.defineProperty(shadowedResponse, "headers", {
        get: function () {
          throw new Error("instance headers getter was called");
        },
      });

      const requestBlobPromise = request.blob();
      const responseBlobPromise = response.blob();
      const shadowedBlobPromise = shadowedResponse.blob();

      return {
        types: [
          requestBlobPromise.type,
          responseBlobPromise.type,
          shadowedBlobPromise.type,
        ],
        isPromise: [
          requestBlobPromise instanceof Promise,
          responseBlobPromise instanceof Promise,
          shadowedBlobPromise instanceof Promise,
        ],
        rawMime: [
          request._rawMimeType,
          response._rawMimeType,
          shadowedResponse._rawMimeType,
        ],
      };
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const actual = JSON.parse(String(r.result?.value));

  // 验证 MIME 序列化保留了 CHARSET=GBK 参数中的大写 GBK
  expect(actual.types).toEqual([
    "text/html;charset=GBK",
    "text/html;charset=GBK",
    "text/html;charset=GBK",
  ]);
  expect(actual.isPromise).toEqual([true, true, true]);
  expect(actual.rawMime).toEqual([
    "text/html;charset=GBK",
    "text/html;charset=GBK",
    "text/html;charset=GBK",
  ]);
});
