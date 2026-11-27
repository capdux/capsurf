// src/js/shim/element/dialog.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/dialog.ts

function installDialogElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  function HTMLDialogElement() {
    if (global.HTMLElement) {
      const el = global.HTMLElement.call(this);
      if (el) return el;
    }
  }
  HTMLDialogElement.prototype = Object.create(HTMLElementProto);
  HTMLDialogElement.prototype.constructor = HTMLDialogElement;

  const returnValues = new WeakMap();
  const modalDialogs = new WeakMap();

  const dialogShow = function () {
    if (this.open) {
      if (modalDialogs.get(this) === true) {
        throw new DOMException("The dialog is already open.", "InvalidStateError");
      }
      return;
    }
    modalDialogs.set(this, false);
    this._isModalDialog = false;
    this.setAttribute("open", "");
  };

  const dialogShowModal = function () {
    if (this.open) {
      if (modalDialogs.get(this) === true) return;
      throw new DOMException("The dialog is already open.", "InvalidStateError");
    }
    if (!this.isConnected) {
      throw new DOMException("The element is not connected.", "InvalidStateError");
    }
    if (this._popoverOpen) {
      throw new DOMException("The dialog is already open as a popover.", "InvalidStateError");
    }
    modalDialogs.set(this, true);
    this._isModalDialog = true;
    this.setAttribute("open", "");
  };

  const dialogClose = function (returnValue) {
    if (!this.open) return;
    if (returnValue !== undefined) {
      this.returnValue = String(returnValue);
    }
    this._isModalDialog = false;
    this.removeAttribute("open");
    modalDialogs.delete(this);
    const dialog = this;
    Promise.resolve().then(function () {
      const ev = new Event("close", { bubbles: false, cancelable: false });
      dialog.dispatchEvent(ev);
    });
  };

  const dialogRequestClose = function (returnValue) {
    if (!this.open) return;
    const cancel = new Event("cancel", { bubbles: false, cancelable: true });
    if (!this.dispatchEvent(cancel)) return;
    dialogClose.call(this, returnValue);
  };

  Object.defineProperties(HTMLDialogElement.prototype, {
    open: {
      configurable: true,
      enumerable: true,
      get: function () {
        return this.hasAttribute ? this.hasAttribute("open") : false;
      },
      set: function (val) {
        if (val) {
          this.setAttribute("open", "");
        } else {
          this.removeAttribute("open");
          modalDialogs.delete(this);
        }
      },
    },
    returnValue: {
      configurable: true,
      enumerable: true,
      get: function () {
        return returnValues.get(this) ?? "";
      },
      set: function (value) {
        returnValues.set(this, String(value));
      },
    },
    show: { configurable: true, writable: true, value: dialogShow },
    showModal: { configurable: true, writable: true, value: dialogShowModal },
    close: { configurable: true, writable: true, value: dialogClose },
    requestClose: { configurable: true, writable: true, value: dialogRequestClose },
  });

  markNative(HTMLDialogElement, "HTMLDialogElement");
  markNative(dialogShow, "show");
  markNative(dialogShowModal, "showModal");
  markNative(dialogClose, "close");
  markNative(dialogRequestClose, "requestClose");

  global.HTMLDialogElement = HTMLDialogElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLDialogElement = HTMLDialogElement;
  }
}
