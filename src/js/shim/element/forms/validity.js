// src/js/shim/element/forms/validity.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/validity.ts

function ValidityState(control) {
  this._control = control;
}

Object.defineProperty(ValidityState.prototype, "valueMissing", {
  get: function () {
    const c = this._control;
    if (!c || !c.required) return false;
    if (c.type === "checkbox" || c.type === "radio") {
      return !c.checked;
    }
    return !c.value || c.value.length === 0;
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "customError", {
  get: function () {
    return Boolean(this._control && this._control._customValidityMessage);
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "typeMismatch", {
  get: function () {
    const c = this._control;
    if (!c || !c.value) return false;
    if (c.type === "email") {
      return !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(c.value);
    }
    if (c.type === "url") {
      try {
        new URL(c.value);
        return false;
      } catch (e) {
        return true;
      }
    }
    return false;
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "patternMismatch", {
  get: function () {
    const c = this._control;
    if (!c || !c.value || !c.pattern) return false;
    try {
      const reg = new RegExp("^(?:" + c.pattern + ")$");
      return !reg.test(c.value);
    } catch (e) {
      return false;
    }
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "tooLong", {
  get: function () {
    const c = this._control;
    if (!c || !c.value || c.maxLength === undefined || c.maxLength < 0) return false;
    return c.value.length > c.maxLength;
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "tooShort", {
  get: function () {
    const c = this._control;
    if (!c || !c.value || c.minLength === undefined || c.minLength <= 0) return false;
    return c.value.length < c.minLength;
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "rangeUnderflow", {
  get: function () {
    const c = this._control;
    if (!c || c.min === undefined || c.min === "" || !c.value) return false;
    const num = Number(c.value);
    const min = Number(c.min);
    return !isNaN(num) && !isNaN(min) && num < min;
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "rangeOverflow", {
  get: function () {
    const c = this._control;
    if (!c || c.max === undefined || c.max === "" || !c.value) return false;
    const num = Number(c.value);
    const max = Number(c.max);
    return !isNaN(num) && !isNaN(max) && num > max;
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "stepMismatch", {
  get: function () { return false; },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "badInput", {
  get: function () {
    const c = this._control;
    if (!c || !c.value) return false;
    if (c.type === "number") return isNaN(Number(c.value));
    return false;
  },
  configurable: true,
});

Object.defineProperty(ValidityState.prototype, "valid", {
  get: function () {
    return !this.valueMissing &&
      !this.customError &&
      !this.typeMismatch &&
      !this.patternMismatch &&
      !this.tooLong &&
      !this.tooShort &&
      !this.rangeUnderflow &&
      !this.rangeOverflow &&
      !this.stepMismatch &&
      !this.badInput;
  },
  configurable: true,
});

markNative(ValidityState, "ValidityState");
