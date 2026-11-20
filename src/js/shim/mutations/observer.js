// src/js/shim/mutations/observer.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/mutations/observer.ts

const _mrState = new WeakMap();
const _moState = new WeakMap();
const _suppressedMutationTargets = new WeakMap();

function suppressChildListMutations(target, fn) {
  const prev = _suppressedMutationTargets.get(target) || 0;
  _suppressedMutationTargets.set(target, prev + 1);
  try {
    return fn();
  } finally {
    if (prev === 0) _suppressedMutationTargets.delete(target);
    else _suppressedMutationTargets.set(target, prev);
  }
}

function MutationRecord(type, target) {
  if (arguments.length === 0) {
    throw new TypeError("Illegal constructor");
  }
  _mrState.set(this, {
    type: type,
    target: target,
    addedNodes: [],
    removedNodes: [],
    previousSibling: null,
    nextSibling: null,
    attributeName: null,
    attributeNamespace: null,
    oldValue: null,
  });
}
["type", "target", "addedNodes", "removedNodes", "previousSibling", "nextSibling", "attributeName", "attributeNamespace", "oldValue"].forEach(function (prop) {
  Object.defineProperty(MutationRecord.prototype, prop, {
    get: function () {
      const st = _mrState.get(this);
      if (!st) throw new TypeError("Illegal invocation");
      return st[prop];
    },
    set: function (v) {
      const st = _mrState.get(this);
      if (st) st[prop] = v;
    },
    configurable: true,
    enumerable: true,
  });
});
markNative(MutationRecord, "MutationRecord");

const activeObservers = new Set();
let mutationScheduled = false;

function scheduleMutationDelivery() {
  if (mutationScheduled) return;
  mutationScheduled = true;
  Promise.resolve().then(function () {
    mutationScheduled = false;
    for (const obs of activeObservers) {
      const st = _moState.get(obs);
      if (st) {
        st.transientTargets = [];
        if (st.records.length > 0) {
          const batch = st.records.splice(0, st.records.length);
          try {
            st.callback.call(obs, batch, obs);
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          }
        }
      }
    }
  });
}

function isSameTarget(a, b) {
  if (a === b) return true;
  if (a && b && a._nid !== undefined && b._nid !== undefined && a._nid === b._nid) return true;
  return false;
}

function matchesObserverRecord(sub, record) {
  if (record.type === "attributes" && sub.options.attributes) {
    return !sub.options.attributeFilter || sub.options.attributeFilter.indexOf(record.attributeName) !== -1;
  }
  if (record.type === "childList" && sub.options.childList) {
    return true;
  }
  if (record.type === "characterData" && sub.options.characterData) {
    return true;
  }
  return false;
}

function notifyMutation(record) {
  if (typeof globalThis !== "undefined") {
    globalThis.__treeSeq = (globalThis.__treeSeq || 0) + 1;
  }
  if (record && record.type === "childList" && record.removedNodes && record.removedNodes.length > 0 && typeof global.__capsurf_onNodeRemovedForIterators === "function") {
    for (let ri = 0; ri < record.removedNodes.length; ri++) {
      global.__capsurf_onNodeRemovedForIterators(record.target, record.removedNodes[ri], record.previousSibling);
    }
  }
  if (record && record.type === "childList" && record.target && _suppressedMutationTargets.has(record.target)) {
    return;
  }
  for (const obs of activeObservers) {
    const st = _moState.get(obs);
    if (!st) continue;
    let matched = false;
    for (const sub of st.targets) {
      if (isSameTarget(sub.target, record.target) || (sub.options.subtree && sub.target.contains && sub.target.contains(record.target))) {
        if (record.type === "childList" && sub.options.subtree && record.removedNodes && record.removedNodes.length > 0) {
          if (!st.transientTargets) st.transientTargets = [];
          for (let ri = 0; ri < record.removedNodes.length; ri++) {
            const rn = record.removedNodes[ri];
            if (rn) st.transientTargets.push({ target: rn, options: sub.options });
          }
          scheduleMutationDelivery();
        }
        if (!matched && matchesObserverRecord(sub, record)) {
          st.records.push(record);
          scheduleMutationDelivery();
          matched = true;
        }
      }
    }
    if (!matched && st.transientTargets && st.transientTargets.length > 0) {
      for (const tsub of st.transientTargets) {
        if (isSameTarget(tsub.target, record.target) || (tsub.options.subtree && tsub.target.contains && tsub.target.contains(record.target))) {
          if (matchesObserverRecord(tsub, record)) {
            st.records.push(record);
            scheduleMutationDelivery();
            break;
          }
        }
      }
    }
  }
}

function MutationObserver(callback) {
  if (typeof callback !== "function") throw new TypeError("Callback required");
  _moState.set(this, {
    callback: callback,
    records: [],
    targets: [],
    transientTargets: [],
  });
}

MutationObserver.prototype.observe = function (target, options) {
  const st = _moState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  if (!target) throw new TypeError("Target required");
  const opts = options || {};
  // Update existing observation of this target if present
  for (let i = 0; i < st.targets.length; i++) {
    if (isSameTarget(st.targets[i].target, target)) {
      st.targets[i].options = opts;
      activeObservers.add(this);
      return;
    }
  }
  st.targets.push({ target: target, options: opts });
  activeObservers.add(this);
};

MutationObserver.prototype.disconnect = function () {
  const st = _moState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  st.records = [];
  st.targets = [];
  st.transientTargets = [];
  activeObservers.delete(this);
};

MutationObserver.prototype.takeRecords = function () {
  const st = _moState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  const records = st.records.slice();
  st.records = [];
  return records;
};

markNative(MutationObserver, "MutationObserver");
markNative(MutationObserver.prototype.observe, "observe");
markNative(MutationObserver.prototype.disconnect, "disconnect");
markNative(MutationObserver.prototype.takeRecords, "takeRecords");
