// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
function graded(model) {
    return model.filter((iface) => !iface.global && !iface.noInterfaceObject);
}
function forgeable(members) {
    return Object.entries(members ?? {})
        .filter(([, member]) => !member.unforgeable)
        .map(([name]) => name);
}
export function buildRequest(model) {
    const interfaces = {};
    const globals = [];
    for (const iface of graded(model)) {
        interfaces[iface.name] = {
            ...(iface.parent ? { parent: iface.parent } : {}),
            constants: Object.keys(iface.constants ?? {}),
            attributes: forgeable(iface.attributes),
            operations: forgeable(iface.operations),
            staticAttributes: forgeable(iface.staticAttributes),
            staticOperations: forgeable(iface.staticOperations),
        };
        globals.push(...(iface.windowAliases ?? []), ...(iface.factoryFunctions ?? []));
    }
    return { interfaces, globals };
}
function hasSetter(attribute) {
    return (!attribute.readonly ||
        !!attribute.putForwards ||
        !!attribute.replaceable ||
        !!attribute.lenientSetter);
}
function gradeConstant(gaps, where, seen, value) {
    if (!seen || seen.k === "-")
        gaps.push(`${where}: missing constant`);
    else if (seen.k === "i")
        gaps.push(`${where}: inherited, not own`);
    else if (seen.k === "a")
        gaps.push(`${where}: accessor, expected a data property`);
    else {
        if (seen.v !== value)
            gaps.push(`${where}: is ${String(seen.v)}, expected ${String(value)}`);
        if (seen.w || seen.c || !seen.e) {
            gaps.push(`${where}: expected non-writable, enumerable, non-configurable`);
        }
    }
}
function gradeAttribute(gaps, where, seen, attribute) {
    if (!seen || seen.k === "-")
        return void gaps.push(`${where}: missing attribute`);
    if (seen.k === "i")
        return void gaps.push(`${where}: inherited, not own`);
    if (seen.k === "d")
        return void gaps.push(`${where}: data property, expected an accessor`);
    if (!seen.g)
        gaps.push(`${where}: no getter`);
    if (hasSetter(attribute) && !seen.s)
        gaps.push(`${where}: no setter`);
    if (!hasSetter(attribute) && seen.s)
        gaps.push(`${where}: has a setter, expected readonly`);
    if (!seen.e)
        gaps.push(`${where}: not enumerable`);
}
function gradeOperation(gaps, where, seen, operation) {
    if (!seen || seen.k === "-")
        return void gaps.push(`${where}: missing operation`);
    if (seen.k === "i")
        return void gaps.push(`${where}: inherited, not own`);
    if (seen.k === "a" || seen.t !== "function") {
        return void gaps.push(`${where}: not a function`);
    }
    if (seen.l !== operation.length) {
        gaps.push(`${where}: length is ${String(seen.l)}, expected ${operation.length}`);
    }
    if (!seen.e)
        gaps.push(`${where}: not enumerable`);
}
export function grade(model, report) {
    const gaps = [];
    for (const iface of graded(model)) {
        const name = iface.name;
        const seen = report.interfaces[name];
        const callbackObject = iface.callback && seen?.type === "object";
        if (!seen || (seen.type !== "function" && !callbackObject)) {
            gaps.push(`${name}: missing interface`);
            continue;
        }
        for (const alias of iface.windowAliases ?? []) {
            if (report.globals[alias] !== "function")
                gaps.push(`${alias}: missing window alias of ${name}`);
        }
        for (const factory of iface.factoryFunctions ?? []) {
            if (report.globals[factory] !== "function")
                gaps.push(`${factory}: missing factory function for ${name}`);
        }
        for (const [constant, value] of Object.entries(iface.constants ?? {})) {
            gradeConstant(gaps, `${name}.${constant}`, seen.constants?.[constant], value);
        }
        if (iface.callback)
            continue;
        if (seen.name !== name)
            gaps.push(`${name}: name is ${JSON.stringify(seen.name)}`);
        if (seen.length !== iface.length) {
            gaps.push(`${name}: length is ${String(seen.length)}, expected ${iface.length}`);
        }
        if (!seen.objectParent) {
            gaps.push(`${name}: interface object does not inherit from ${iface.parent ?? "Function.prototype"}`);
        }
        for (const [attribute, idl] of Object.entries(iface.staticAttributes ?? {})) {
            if (!idl.unforgeable)
                gradeAttribute(gaps, `${name}.${attribute}`, seen.staticAttributes?.[attribute], idl);
        }
        for (const [operation, idl] of Object.entries(iface.staticOperations ?? {})) {
            if (!idl.unforgeable)
                gradeOperation(gaps, `${name}.${operation}()`, seen.staticOperations?.[operation], idl);
        }
        if (!seen.hasPrototype) {
            gaps.push(`${name}.prototype: missing`);
            continue;
        }
        if (!seen.prototypeParent) {
            gaps.push(`${name}.prototype: does not inherit from ${iface.parent ? `${iface.parent}.prototype` : "Object.prototype"}`);
        }
        if (seen.classString !== `[object ${name}]`) {
            gaps.push(`${name}.prototype: class string is ${JSON.stringify(seen.classString)}`);
        }
        for (const [constant, value] of Object.entries(iface.constants ?? {})) {
            gradeConstant(gaps, `${name}.prototype.${constant}`, seen.prototypeConstants?.[constant], value);
        }
        for (const [attribute, idl] of Object.entries(iface.attributes ?? {})) {
            if (!idl.unforgeable)
                gradeAttribute(gaps, `${name}.prototype.${attribute}`, seen.attributes?.[attribute], idl);
        }
        for (const [operation, idl] of Object.entries(iface.operations ?? {})) {
            if (!idl.unforgeable)
                gradeOperation(gaps, `${name}.prototype.${operation}()`, seen.operations?.[operation], idl);
        }
        if ((iface.iterable || iface.setlike || iface.maplike) && !seen.iterator) {
            gaps.push(`${name}.prototype[Symbol.iterator]: missing`);
        }
    }
    return gaps.sort();
}
