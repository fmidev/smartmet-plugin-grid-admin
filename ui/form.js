// Dialogs for editing a record and for confirming an action. The page never uses the
// blocking alert/confirm of the browser.

import {el} from './dom.js';

// A form in a modal dialog. fields: [{name, label, type: 'text'|'int'|'number'|'time'|'select',
// options: [{value, label}], help, required}]. Resolves to the values, or null when cancelled.
// The values are sent as strings; the server checks them.
export function editDialog(title, fields, values = {}, {submitLabel = 'Save', note = ''} = {})
{
  return new Promise((resolve) => {
    const inputs = {};
    const rows = fields.map((f) => {
      const id = 'field-' + f.name;
      let input;
      const value = values[f.name] ?? '';
      if (f.type === 'select')
      {
        input = el('select', {id}, f.options.map((o) =>
          el('option', {value: o.value, selected: String(o.value) === String(value)}, o.label)));
      }
      else
      {
        input = el('input', {
          id,
          type: 'text',
          value: String(value),
          inputmode: f.type === 'int' || f.type === 'number' ? 'decimal' : null,
          pattern: f.type === 'int' ? '-?[0-9]+' : f.type === 'time' ? '([0-9]{8}T[0-9]{6})?' : null,
          placeholder: f.type === 'time' ? 'YYYYMMDDTHHMMSS' : null,
          required: f.required || null,
          spellcheck: 'false',
        });
      }
      inputs[f.name] = input;
      return [
        el('label', {for: id}, f.label),
        el('div', {}, input, f.help ? el('div', {class: 'muted small'}, f.help) : null),
      ];
    });

    const message = el('p', {class: 'error', role: 'alert'});
    const cancel = el('button', {type: 'button'}, 'Cancel');
    const submit = el('button', {type: 'submit', class: 'primary'}, submitLabel);
    const form = el('form', {method: 'dialog'},
      el('h2', {}, title),
      note ? el('p', {class: 'muted small'}, note) : null,
      el('div', {class: 'form-grid'}, rows),
      message,
      el('div', {class: 'dialog-buttons'}, cancel, submit));
    const dialog = el('dialog', {class: 'dialog'}, form);

    const close = (result) => {
      dialog.close();
      dialog.remove();
      resolve(result);
    };
    cancel.addEventListener('click', () => close(null));
    dialog.addEventListener('cancel', (e) => {
      e.preventDefault();
      close(null);
    });
    form.addEventListener('submit', (e) => {
      e.preventDefault();
      if (!form.reportValidity())
        return;
      const result = {};
      for (const f of fields)
        result[f.name] = inputs[f.name].value.trim();
      close(result);
    });

    document.body.append(dialog);
    dialog.showModal();
    const first = form.querySelector('input, select');
    first?.focus();
  });
}

// A confirmation. Resolves to true or false.
export function confirmDialog(title, text, {confirmLabel = 'Delete', danger = true} = {})
{
  return new Promise((resolve) => {
    const cancel = el('button', {type: 'button'}, 'Cancel');
    const ok = el('button', {type: 'button', class: danger ? 'danger' : 'primary'}, confirmLabel);
    const dialog = el('dialog', {class: 'dialog'},
      el('h2', {}, title), el('p', {}, text), el('div', {class: 'dialog-buttons'}, cancel, ok));
    const close = (result) => {
      dialog.close();
      dialog.remove();
      resolve(result);
    };
    cancel.addEventListener('click', () => close(false));
    ok.addEventListener('click', () => close(true));
    dialog.addEventListener('cancel', (e) => {
      e.preventDefault();
      close(false);
    });
    document.body.append(dialog);
    dialog.showModal();
    cancel.focus();
  });
}

// A short message at the bottom of the page
export function toast(text, {error = false} = {})
{
  const node = el('div', {class: 'toast' + (error ? ' toast-error' : ''), role: 'status'}, text);
  document.body.append(node);
  setTimeout(() => node.remove(), error ? 8000 : 4000);
}
