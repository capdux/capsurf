// src/js/shim/mutations/observer.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/mutations/observer.ts

function MutationRecord(type, target) {
  this.type = type; // "childList", "attributes", "characterData"
  this.target = target;
  this.addedNodes = [];
  this.removedNodes = [];
  this.previousSibling = null;
  this.nextSibling = null;
  this.attributeName = null;
  this.attributeNamespace = null;
  this.oldValue = null;
}
markNative(MutationRecord, "MutationRecord");

const activeObservers = new Set();
let mutationScheduled = false;

function scheduleMutationDelivery() {
  if (mutationScheduled) return;
  mutationScheduled = true;
  Promise.resolve().then(function () {
    mutationScheduled = false;
    for (const obs of activeObservers) {
      if (obs._records.length > 0) {
        const batch = obs._records.splice(0, obs._records.length);
        try {
          obs.callback.call(obs, batch, obs);
        } catch (e) {
          if (typeof console !== "undefined" && console.error) console.error(e);
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

function notifyMutation(record) {
  if (typeof globalThis !== "undefined") {
    globalThis.__treeSeq = (globalThis.__treeSeq || 0) + 1;
  }
  for (const obs of activeObservers) {
    for (const sub of obs._targets) {
      if (isSameTarget(sub.target, record.target) || (sub.options.subtree && sub.target.contains && sub.target.contains(record.target))) {
        if (record.type === "attributes" && sub.options.attributes) {
          if (!sub.options.attributeFilter || sub.options.attributeFilter.indexOf(record.attributeName) !== -1) {
            obs._records.push(record);
            scheduleMutationDelivery();
            break;
          }
        } else if (record.type === "childList" && sub.options.childList) {
          obs._records.push(record);
          scheduleMutationDelivery();
          break;
        } else if (record.type === "characterData" && sub.options.characterData) {
          obs._records.push(record);
          scheduleMutationDelivery();
          break;
        }
      }
    }
  }
}

function MutationObserver(callback) {
  if (typeof callback !== "function") throw new TypeError("Callback required");
  this.callback = callback;
  this._records = [];
  this._targets = [];
}

MutationObserver.prototype.observe = function (target, options) {
  if (!target) throw new TypeError("Target required");
  const opts = options || {};
  // Update existing observation of this target if present
  for (let i = 0; i < this._targets.length; i++) {
    if (isSameTarget(this._targets[i].target, target)) {
      this._targets[i].options = opts;
      activeObservers.add(this);
      return;
    }
  }
  this._targets.push({ target: target, options: opts });
  activeObservers.add(this);
};

MutationObserver.prototype.disconnect = function () {
  this._records = [];
  this._targets = [];
  activeObservers.delete(this);
};

MutationObserver.prototype.takeRecords = function () {
  const records = this._records.slice();
  this._records = [];
  return records;
};

markNative(MutationObserver, "MutationObserver");
markNative(MutationObserver.prototype.observe, "observe");
markNative(MutationObserver.prototype.disconnect, "disconnect");
markNative(MutationObserver.prototype.takeRecords, "takeRecords");
