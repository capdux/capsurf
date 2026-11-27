// src/js/shim/element/forms/validity.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/validity.ts & input-types.ts

const MS_PER_SECOND = 1000;
const MS_PER_MINUTE = 60 * MS_PER_SECOND;
const MS_PER_HOUR = 60 * MS_PER_MINUTE;
const MS_PER_DAY = 24 * MS_PER_HOUR;
const MS_PER_WEEK = 7 * MS_PER_DAY;
const STEP_ALIGNMENT_EPSILON = 1e-9;
const MONTH_VALUE_EPOCH_YEAR = 1970;

const EMAIL_RE =
  /^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/;

const RX_DATE = /^(\d{4,})-(\d{2})-(\d{2})$/;
const RX_MONTH = /^(\d{4,})-(\d{2})$/;
const RX_WEEK = /^(\d{4,})-W(\d{2})$/;
const RX_TIME = /^(\d{2}):(\d{2})(?::(\d{2})(?:\.(\d{1,3}))?)?$/;
const RX_DATETIME_LOCAL =
  /^(\d{4,})-(\d{2})-(\d{2})[T ](\d{2}):(\d{2})(?::(\d{2})(?:\.(\d{1,3}))?)?$/;
const RX_NUMBER = /^-?(?:\d+(?:\.\d+)?|\.\d+)(?:[eE][+-]?\d+)?$/;

function isSteppableInputType(type) {
  return (
    type === "number" ||
    type === "range" ||
    type === "date" ||
    type === "month" ||
    type === "week" ||
    type === "time" ||
    type === "datetime-local"
  );
}

function acceptsValueAsDateType(type) {
  return type === "date" || type === "month" || type === "week" || type === "time";
}

function stepDefaultForType(type) {
  switch (type) {
    case "date":
      return MS_PER_DAY;
    case "month":
      return 1;
    case "week":
      return MS_PER_WEEK;
    case "time":
    case "datetime-local":
      return MS_PER_MINUTE;
    case "number":
    case "range":
    default:
      return 1;
  }
}

function stepScaleForType(type) {
  switch (type) {
    case "date":
      return MS_PER_DAY;
    case "week":
      return MS_PER_WEEK;
    case "time":
    case "datetime-local":
      return MS_PER_SECOND;
    case "month":
    case "number":
    case "range":
    default:
      return 1;
  }
}

function stepBaseDefaultForType(type) {
  if (type === "week") return -3 * MS_PER_DAY;
  return 0;
}

function utcMs(y, mo0, d, h, mi, s, ms) {
  const dt = new Date(0);
  dt.setUTCFullYear(y, mo0, d);
  dt.setUTCHours(h || 0, mi || 0, s || 0, ms || 0);
  return dt.getTime();
}

function daysInMonthUTC(year, month1) {
  return new Date(utcMs(year, month1, 0)).getUTCDate();
}

function isValidYMD(y, m, d) {
  if (y < 1) return false;
  if (m < 1 || m > 12) return false;
  if (d < 1 || d > daysInMonthUTC(y, m)) return false;
  return true;
}

function parseInputDate(value) {
  const m = RX_DATE.exec(value);
  if (!m) return null;
  const y = +m[1];
  const mo = +m[2];
  const d = +m[3];
  if (m[1].length < 4 || !isValidYMD(y, mo, d)) return null;
  return utcMs(y, mo - 1, d);
}

function parseInputMonth(value) {
  const m = RX_MONTH.exec(value);
  if (!m) return null;
  const y = +m[1];
  const mo = +m[2];
  if (m[1].length < 4 || y < 1 || mo < 1 || mo > 12) return null;
  return (y - MONTH_VALUE_EPOCH_YEAR) * 12 + (mo - 1);
}

function weeksInISOYear(year) {
  const jan1 = new Date(utcMs(year, 0, 1)).getUTCDay();
  const isLeap = (year % 4 === 0 && year % 100 !== 0) || year % 400 === 0;
  if (jan1 === 4 || (isLeap && jan1 === 3)) return 53;
  return 52;
}

function isoWeekMondayUTC(year, week) {
  const jan4 = utcMs(year, 0, 4);
  const jan4Day = new Date(jan4).getUTCDay();
  const isoDay = jan4Day === 0 ? 7 : jan4Day;
  const week1Monday = jan4 - (isoDay - 1) * MS_PER_DAY;
  return week1Monday + (week - 1) * MS_PER_WEEK;
}

function parseInputWeek(value) {
  const m = RX_WEEK.exec(value);
  if (!m) return null;
  const y = +m[1];
  const w = +m[2];
  if (m[1].length < 4 || y < 1) return null;
  if (w < 1 || w > weeksInISOYear(y)) return null;
  return isoWeekMondayUTC(y, w);
}

function parseInputTime(value) {
  const m = RX_TIME.exec(value);
  if (!m) return null;
  const h = +m[1];
  const mi = +m[2];
  const s = m[3] ? +m[3] : 0;
  const ms = m[4] ? +m[4].padEnd(3, "0") : 0;
  if (h > 23 || mi > 59 || s > 59) return null;
  return h * MS_PER_HOUR + mi * MS_PER_MINUTE + s * MS_PER_SECOND + ms;
}

function parseInputDateTimeLocal(value) {
  const m = RX_DATETIME_LOCAL.exec(value);
  if (!m) return null;
  const y = +m[1];
  const mo = +m[2];
  const d = +m[3];
  const h = +m[4];
  const mi = +m[5];
  const s = m[6] ? +m[6] : 0;
  const ms = m[7] ? +m[7].padEnd(3, "0") : 0;
  if (m[1].length < 4 || !isValidYMD(y, mo, d)) return null;
  if (h > 23 || mi > 59 || s > 59) return null;
  return utcMs(y, mo - 1, d, h, mi, s, ms);
}

function parseInputNumber(value) {
  if (!RX_NUMBER.test(value)) return null;
  const n = +value;
  return Number.isFinite(n) ? n : null;
}

function parseInputByType(type, value) {
  if (value === "" || value == null) return null;
  const s = String(value);
  switch (type) {
    case "date":
      return parseInputDate(s);
    case "month":
      return parseInputMonth(s);
    case "week":
      return parseInputWeek(s);
    case "time":
      return parseInputTime(s);
    case "datetime-local":
      return parseInputDateTimeLocal(s);
    case "number":
    case "range":
      return parseInputNumber(s);
    default:
      return null;
  }
}

function padNum(n, w) {
  const s = String(Math.abs(Math.trunc(n)));
  return (n < 0 ? "-" : "") + (s.length >= w ? s : "0".repeat(w - s.length) + s);
}

function serializeInputDate(ms) {
  const d = new Date(Math.floor(ms / MS_PER_DAY) * MS_PER_DAY);
  return padNum(d.getUTCFullYear(), 4) + "-" + padNum(d.getUTCMonth() + 1, 2) + "-" + padNum(d.getUTCDate(), 2);
}

function serializeInputMonth(nMonths) {
  const total = Math.floor(nMonths);
  const y = MONTH_VALUE_EPOCH_YEAR + Math.floor(total / 12);
  const m = (((total % 12) + 12) % 12) + 1;
  return padNum(y, 4) + "-" + padNum(m, 2);
}

function serializeInputWeek(ms) {
  const d = new Date(ms);
  const day = d.getUTCDay() || 7;
  const thursday = new Date(d.getTime());
  thursday.setUTCDate(d.getUTCDate() + (4 - day));
  const y = thursday.getUTCFullYear();
  const yStart = utcMs(y, 0, 4);
  const yStartDay = new Date(yStart).getUTCDay() || 7;
  const week1Monday = yStart - (yStartDay - 1) * MS_PER_DAY;
  const w = Math.round((thursday.getTime() - week1Monday) / MS_PER_WEEK) + 1;
  return padNum(y, 4) + "-W" + padNum(w, 2);
}

function serializeInputTime(ms) {
  if (!Number.isFinite(ms)) return "";
  const norm = ((ms % MS_PER_DAY) + MS_PER_DAY) % MS_PER_DAY;
  const h = Math.floor(norm / MS_PER_HOUR);
  const mi = Math.floor((norm % MS_PER_HOUR) / MS_PER_MINUTE);
  const s = Math.floor((norm % MS_PER_MINUTE) / MS_PER_SECOND);
  const frac = norm % MS_PER_SECOND;
  let out = padNum(h, 2) + ":" + padNum(mi, 2);
  if (s || frac) out += ":" + padNum(s, 2);
  if (frac) out += "." + padNum(frac, 3);
  return out;
}

function serializeInputDateTimeLocal(ms) {
  const d = new Date(ms);
  const y = d.getUTCFullYear();
  const mo = d.getUTCMonth() + 1;
  const day = d.getUTCDate();
  const h = d.getUTCHours();
  const mi = d.getUTCMinutes();
  const s = d.getUTCSeconds();
  const frac = d.getUTCMilliseconds();
  let out = padNum(y, 4) + "-" + padNum(mo, 2) + "-" + padNum(day, 2) + "T" + padNum(h, 2) + ":" + padNum(mi, 2);
  if (s || frac) out += ":" + padNum(s, 2);
  if (frac) out += "." + padNum(frac, 3);
  return out;
}

function serializeInputByType(type, n) {
  switch (type) {
    case "date":
      return serializeInputDate(n);
    case "month":
      return serializeInputMonth(n);
    case "week":
      return serializeInputWeek(n);
    case "time":
      return serializeInputTime(n);
    case "datetime-local":
      return serializeInputDateTimeLocal(n);
    case "number":
    case "range":
      return String(n);
    default:
      return String(n);
  }
}

function valueAsDateFromString(type, value) {
  if (!acceptsValueAsDateType(type)) return null;
  const n = parseInputByType(type, value);
  if (n == null) return null;
  if (type === "month") return new Date(utcMs(MONTH_VALUE_EPOCH_YEAR, n, 1));
  return new Date(n);
}

function valueAsDateToString(type, d) {
  if (!acceptsValueAsDateType(type)) return null;
  if (d === null) return "";
  if (!(d instanceof Date) || isNaN(d.getTime())) return "";
  const out =
    type === "month"
      ? serializeInputMonth((d.getUTCFullYear() - MONTH_VALUE_EPOCH_YEAR) * 12 + d.getUTCMonth())
      : serializeInputByType(type, d.getTime());
  return parseInputByType(type, out) == null ? "" : out;
}

function resolvedInputStep(type, stepAttr) {
  if (stepAttr != null && String(stepAttr).toLowerCase() === "any") return null;
  if (stepAttr == null || stepAttr === "") return stepDefaultForType(type);
  const n = +stepAttr;
  if (!Number.isFinite(n) || n <= 0) return stepDefaultForType(type);
  return n * stepScaleForType(type);
}

function resolvedInputStepBase(type, minAttr, valueAttr) {
  if (minAttr != null && minAttr !== "") {
    const m = parseInputByType(type, minAttr);
    if (m != null) return m;
  }
  if (valueAttr != null && valueAttr !== "") {
    const v = parseInputByType(type, valueAttr);
    if (v != null) return v;
  }
  return stepBaseDefaultForType(type);
}

function stripInputNewlines(s) {
  return s.indexOf("\n") === -1 && s.indexOf("\r") === -1 ? s : s.replace(/[\n\r]+/g, "");
}

function stripInputHtmlWhitespace(s) {
  return s.replace(/^[ \t\n\f\r]+|[ \t\n\f\r]+$/g, "");
}

function sanitizeInputValue(type, value, opts) {
  const str = String(value == null ? "" : value);
  switch (type) {
    case "text":
    case "search":
    case "tel":
    case "password":
      return stripInputNewlines(str);
    case "url":
      return stripInputHtmlWhitespace(stripInputNewlines(str));
    case "email":
      if (opts && opts.multiple) {
        return stripInputNewlines(str).split(",").map(stripInputHtmlWhitespace).join(",");
      }
      return stripInputHtmlWhitespace(stripInputNewlines(str));
    case "number":
      return parseInputNumber(str) == null ? "" : str;
    case "date":
    case "month":
    case "week":
    case "time":
    case "datetime-local":
      return parseInputByType(type, str) == null ? "" : str;
    case "range": {
      let min = 0;
      let max = 100;
      if (opts && opts.minAttr != null && opts.minAttr !== "") {
        const m = parseInputNumber(opts.minAttr);
        if (m != null) min = m;
      }
      if (opts && opts.maxAttr != null && opts.maxAttr !== "") {
        const m = parseInputNumber(opts.maxAttr);
        if (m != null) max = m;
      }
      let n = parseInputNumber(str);
      if (n == null) n = max < min ? min : min + (max - min) / 2;
      if (n > max) n = max;
      if (n < min) n = min;
      const step = resolvedInputStep(type, opts && opts.stepAttr != null ? opts.stepAttr : null);
      if (step != null && max >= min) {
        const base = resolvedInputStepBase(type, opts && opts.minAttr != null ? opts.minAttr : null, null);
        const off = (n - base) / step;
        if (Math.abs(off - Math.round(off)) > STEP_ALIGNMENT_EPSILON) {
          const low = base + Math.floor(off) * step;
          const high = base + Math.ceil(off) * step;
          let cand = n - low < high - n ? low : high;
          if (cand > max) cand = low;
          if (cand < min) cand = high;
          if (cand >= min && cand <= max) {
            n = Number(cand.toPrecision(15));
          }
        }
      }
      return String(n);
    }
    case "color": {
      const s = stripInputNewlines(str);
      return /^#[0-9a-fA-F]{6}$/.test(s) ? s.toLowerCase() : "#000000";
    }
    default:
      return str;
  }
}

function computeInputStep(type, currentValue, minAttr, maxAttr, stepAttr, n, valueAttr) {
  if (!isSteppableInputType(type)) return { error: "InvalidStateError" };
  const step = resolvedInputStep(type, stepAttr);
  if (step == null) return { error: "InvalidStateError" };
  const base = resolvedInputStepBase(type, minAttr, valueAttr);
  let min = null;
  let max = null;
  if (minAttr != null && minAttr !== "") min = parseInputByType(type, minAttr);
  if (maxAttr != null && maxAttr !== "") max = parseInputByType(type, maxAttr);
  if (type === "range") {
    if (min == null) min = 0;
    if (max == null) max = 100;
  }

  let current = parseInputByType(type, currentValue);
  if (current == null) {
    current = 0;
    if (min != null && current < min) current = min;
    if (max != null && current > max) current = max;
  }

  const offset = (current - base) / step;
  const stepEps = Math.max(STEP_ALIGNMENT_EPSILON, Math.abs(offset) * 1e-12);
  let next;
  if (n === 0) {
    return { noop: true };
  }
  if (Math.abs(offset - Math.round(offset)) > stepEps) {
    if (n > 0) {
      next = base + (Math.ceil(offset - stepEps) + (n - 1)) * step;
    } else {
      next = base + (Math.floor(offset + stepEps) + (n + 1)) * step;
    }
  } else {
    next = current + n * step;
  }
  if (max != null && next > max) {
    const overshoot = Math.ceil((next - max) / step - stepEps);
    next = next - overshoot * step;
  }
  if (min != null && next < min) {
    const undershoot = Math.ceil((min - next) / step - stepEps);
    next = next + undershoot * step;
  }

  if (type === "number" || type === "range") {
    next = Number(next.toPrecision(12));
  } else {
    next = Math.round(next);
  }
  if ((min != null && next < min) || (max != null && next > max)) {
    return { noop: true };
  }
  if (n > 0 && next < current) return { noop: true };
  if (n < 0 && next > current) return { noop: true };
  return { value: serializeInputByType(type, next) };
}

function isValidEmailAddress(s) {
  if (!s) return false;
  return EMAIL_RE.test(s);
}

function isValidUrlString(s) {
  if (!s) return false;
  try {
    new URL(s);
    return true;
  } catch (e) {
    return false;
  }
}

function stripValidityAsciiWhitespace(value) {
  return value.replace(/\r\n|\r|\n/g, "").replace(/^[\t\n\f\r ]+|[\t\n\f\r ]+$/g, "");
}

function isDisabledByAncestorFieldset(control) {
  let prev = control;
  let p = control ? (control.parentNode || control.parentElement) : null;
  while (p) {
    const pTag = (p.tagName || p.localName || "").toLowerCase();
    if (pTag === "fieldset" && (p.disabled || (typeof p.hasAttribute === "function" && p.hasAttribute("disabled")))) {
      // Check if prev is the first <legend> child of this fieldset
      let firstLegend = null;
      let child = p.firstChild;
      while (child) {
        if (child.nodeType === 1 && (child.tagName || child.localName || "").toLowerCase() === "legend") {
          firstLegend = child;
          break;
        }
        child = child.nextSibling;
      }
      if (!firstLegend || prev !== firstLegend) {
        return true;
      }
    }
    prev = p;
    p = p.parentNode || p.parentElement;
  }
  return false;
}

function hasAncestorDatalist(control) {
  let p = control ? (control.parentNode || control.parentElement) : null;
  while (p) {
    const pTag = (p.tagName || p.localName || "").toLowerCase();
    if (pTag === "datalist") return true;
    p = p.parentNode || p.parentElement;
  }
  return false;
}

function _isControlDisabled(control) {
  if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_isElementDisabled === "function") {
    return globalThis.__capsurf_isElementDisabled(control);
  }
  return Boolean(control.disabled);
}

function computeWillValidate(control) {
  if (!control) return false;
  const tag = (control.tagName || control.localName || "").toLowerCase();
  if (tag === "input") {
    const type = (control.type || "text").toLowerCase();
    if (
      type === "hidden" ||
      type === "reset" ||
      type === "submit" ||
      type === "button" ||
      type === "image"
    ) {
      return false;
    }
    if (_isControlDisabled(control) || control.readOnly) return false;
  } else if (tag === "textarea") {
    if (_isControlDisabled(control) || control.readOnly) return false;
  } else if (tag === "select") {
    if (_isControlDisabled(control)) return false;
  } else if (tag === "button") {
    if (_isControlDisabled(control)) return false;
    if ((control.type || "submit").toLowerCase() !== "submit") return false;
  } else {
    return false;
  }
  if (isDisabledByAncestorFieldset(control)) return false;
  if (hasAncestorDatalist(control)) return false;
  return true;
}

function compilePatternRegex(pattern) {
  try {
    new RegExp(pattern, "v");
    return new RegExp("^(?:" + pattern + ")$", "v");
  } catch (e1) {
    try {
      new RegExp(pattern, "u");
      return new RegExp("^(?:" + pattern + ")$", "u");
    } catch (e2) {
      try {
        return new RegExp("^(?:" + pattern + ")$");
      } catch (e3) {
        return null;
      }
    }
  }
}

function computeValidityFlags(control) {
  const flags = {
    valueMissing: false,
    typeMismatch: false,
    patternMismatch: false,
    tooLong: false,
    tooShort: false,
    rangeUnderflow: false,
    rangeOverflow: false,
    stepMismatch: false,
    badInput: false,
    customError: false,
    valid: true,
  };
  if (!control) return flags;
  if (control._customValidityMessage) {
    flags.customError = true;
  }

  const tag = (control.tagName || control.localName || "").toLowerCase();
  if (tag === "input") {
    const type = (control.type || "text").toLowerCase();
    const value = control.value || "";
    const required = Boolean(control.required);
    if (
      required &&
      type !== "submit" &&
      type !== "reset" &&
      type !== "button" &&
      type !== "image"
    ) {
      if (type === "checkbox" || type === "radio") {
        if (!control.checked) flags.valueMissing = true;
      } else if (!value) {
        flags.valueMissing = true;
      }
    }

    // typeMismatch
    if (value) {
      if (type === "email") {
        if (control.multiple) {
          const parts = value.split(",").map(stripValidityAsciiWhitespace);
          if (parts.some(function (part) { return !isValidEmailAddress(part); })) {
            flags.typeMismatch = true;
          }
        } else if (!isValidEmailAddress(stripValidityAsciiWhitespace(value))) {
          flags.typeMismatch = true;
        }
      } else if (type === "url") {
        if (!isValidUrlString(stripValidityAsciiWhitespace(value))) {
          flags.typeMismatch = true;
        }
      }
    }

    // patternMismatch
    const pattern = typeof control.getAttribute === "function" ? control.getAttribute("pattern") : control.pattern;
    if (value && pattern) {
      const re = compilePatternRegex(pattern);
      if (re) {
        const mismatch =
          type === "email" && control.multiple
            ? value.split(",").map(stripValidityAsciiWhitespace).some(function (part) { return !re.test(part); })
            : !re.test(value);
        if (mismatch) flags.patternMismatch = true;
      }
    }

    // tooLong / tooShort (only when value was user-edited per HTML spec)
    if (control._userEdited) {
      const ml = control.maxLength;
      if (typeof ml === "number" && ml >= 0 && value.length > ml) {
        flags.tooLong = true;
      }
      const mn = control.minLength;
      if (typeof mn === "number" && mn >= 0 && value && value.length < mn) {
        flags.tooShort = true;
      }
    }

    // numeric / date / time validity
    if (isSteppableInputType(type) && value) {
      const num = parseInputByType(type, value);
      if (num == null) {
        flags.badInput = true;
      } else {
        const minRaw = typeof control.getAttribute === "function" ? control.getAttribute("min") : null;
        const maxRaw = typeof control.getAttribute === "function" ? control.getAttribute("max") : null;
        const stepRaw = typeof control.getAttribute === "function" ? control.getAttribute("step") : null;
        const minN = minRaw != null && minRaw !== "" ? parseInputByType(type, minRaw) : null;
        const maxN = maxRaw != null && maxRaw !== "" ? parseInputByType(type, maxRaw) : null;
        if (minN != null && num < minN) flags.rangeUnderflow = true;
        if (maxN != null && num > maxN) flags.rangeOverflow = true;
        if (stepRaw == null || String(stepRaw).toLowerCase() !== "any") {
          let step = null;
          if (stepRaw == null || stepRaw === "") {
            step = stepDefaultForType(type);
          } else {
            const parsedStep = +stepRaw;
            if (Number.isFinite(parsedStep) && parsedStep > 0) {
              step = parsedStep * stepScaleForType(type);
            }
          }
          if (step != null && step > 0) {
            const base = minN != null ? minN : (type === "week" ? -3 * MS_PER_DAY : 0);
            const diff = (num - base) / step;
            if (Math.abs(diff - Math.round(diff)) > STEP_ALIGNMENT_EPSILON) {
              flags.stepMismatch = true;
            }
          }
        }
      }
    }
  } else if (tag === "textarea") {
    if (!_isControlDisabled(control) && !control.readOnly) {
      const value = control.value || "";
      if (control.required && !value) flags.valueMissing = true;
      if (control._userEdited) {
        const ml = control.maxLength;
        if (typeof ml === "number" && ml >= 0 && value.length > ml) flags.tooLong = true;
        const mn = control.minLength;
        if (typeof mn === "number" && mn >= 0 && value && value.length < mn) flags.tooShort = true;
      }
    }
  } else if (tag === "select") {
    if (control.required && (control.value === "" || control.value == null)) {
      flags.valueMissing = true;
    }
  }

  flags.valid = !(
    flags.valueMissing ||
    flags.typeMismatch ||
    flags.patternMismatch ||
    flags.tooLong ||
    flags.tooShort ||
    flags.rangeUnderflow ||
    flags.rangeOverflow ||
    flags.stepMismatch ||
    flags.badInput ||
    flags.customError
  );
  return flags;
}

function computeValidationMessage(control) {
  if (!computeWillValidate(control)) return "";
  const flags = computeValidityFlags(control);
  if (flags.valid) return "";
  if (flags.customError && control._customValidityMessage) return control._customValidityMessage;
  if (flags.valueMissing) {
    const tag = (control.tagName || control.localName || "").toLowerCase();
    return tag === "select" ? "Please select an item in the list." : "Please fill out this field.";
  }
  if (flags.typeMismatch) return "Please enter a valid value.";
  if (flags.patternMismatch) return "Please match the requested format.";
  if (flags.tooLong) return "Please shorten this text.";
  if (flags.tooShort) return "Please lengthen this text.";
  if (flags.rangeUnderflow) return "Value is below the minimum.";
  if (flags.rangeOverflow) return "Value is above the maximum.";
  if (flags.stepMismatch) return "Please enter a valid value.";
  if (flags.badInput) return "Please enter a valid value.";
  return "";
}

const _validityControlMap = new WeakMap();

function ValidityState(control) {
  _validityControlMap.set(this, control);
}

Object.defineProperty(ValidityState.prototype, Symbol.toStringTag, {
  value: "ValidityState",
  configurable: true,
});

const VALIDITY_FLAG_NAMES = [
  "valueMissing",
  "typeMismatch",
  "patternMismatch",
  "tooLong",
  "tooShort",
  "rangeUnderflow",
  "rangeOverflow",
  "stepMismatch",
  "badInput",
  "customError",
  "valid",
];

for (let i = 0; i < VALIDITY_FLAG_NAMES.length; i++) {
  (function (flagName) {
    Object.defineProperty(ValidityState.prototype, flagName, {
      get: function () {
        const ctrl = _validityControlMap.get(this);
        if (!ctrl) throw new TypeError("Illegal invocation");
        return computeValidityFlags(ctrl)[flagName];
      },
      configurable: true,
      enumerable: true,
    });
  })(VALIDITY_FLAG_NAMES[i]);
}

const _validityStateCache = new WeakMap();
function makeValidityState(control) {
  if (!control || typeof control !== "object") return new ValidityState(control);
  let st = _validityStateCache.get(control);
  if (!st) {
    st = new ValidityState(control);
    _validityStateCache.set(control, st);
  }
  return st;
}

markNative(ValidityState, "ValidityState");
if (typeof globalThis !== "undefined") {
  globalThis.ValidityState = ValidityState;
}
if (typeof global !== "undefined") {
  global.ValidityState = ValidityState;
}
