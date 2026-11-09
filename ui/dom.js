// Small DOM helpers. Text is always set with textContent, never as HTML.

// el('div', {class: 'x', onclick: f}, child, 'text', ...)
export function el(tag, attrs = {}, ...children)
{
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs || {}))
  {
    if (value === undefined || value === null || value === false)
      continue;
    if (key.startsWith('on') && typeof value === 'function')
      node.addEventListener(key.slice(2), value);
    else if (key === 'class')
      node.className = value;
    else if (key === 'dataset')
      Object.assign(node.dataset, value);
    else if (key === 'style' && typeof value === 'object')
      Object.assign(node.style, value);
    else if (value === true)
      node.setAttribute(key, '');
    else
      node.setAttribute(key, String(value));
  }
  append(node, children);
  return node;
}

export function append(node, children)
{
  for (const child of children.flat(Infinity))
  {
    if (child === undefined || child === null || child === false)
      continue;
    node.append(child instanceof Node ? child : document.createTextNode(String(child)));
  }
  return node;
}

export function clear(node)
{
  while (node.firstChild)
    node.removeChild(node.firstChild);
  return node;
}

export function replace(node, ...children)
{
  return append(clear(node), children);
}

// A <select> with the given options [{value, label}] or [value, ...]
export function select(options, selected, onchange, attrs = {})
{
  const node = el('select', attrs);
  for (const option of options)
  {
    const o = typeof option === 'object' ? option : {value: option, label: String(option)};
    const opt = el('option', {value: o.value}, o.label);
    if (String(o.value) === String(selected))
      opt.selected = true;
    node.append(opt);
  }
  if (onchange)
    node.addEventListener('change', () => onchange(node.value));
  return node;
}

// Calls f once the calls have stopped for wait milliseconds
export function debounce(f, wait)
{
  let timer = null;
  const debounced = (...args) => {
    clearTimeout(timer);
    timer = setTimeout(() => f(...args), wait);
  };
  debounced.cancel = () => clearTimeout(timer);
  return debounced;
}

// A table: columns [{label, class, value: row => node|text}], rows, row options
export function table(columns, rows, {onclick = null, selected = null, empty = 'Nothing to show'} = {})
{
  const head = el('tr', {}, columns.map((c) => el('th', {class: c.class, title: c.title}, c.label)));
  const body = el('tbody');
  for (const row of rows)
  {
    const tr = el('tr', {}, columns.map((c) => el('td', {class: c.class}, c.value(row))));
    if (onclick)
    {
      tr.classList.add('clickable');
      tr.tabIndex = 0;
      tr.addEventListener('click', (e) => {
        if (!e.target.closest('a, button'))
          onclick(row);
      });
      tr.addEventListener('keydown', (e) => {
        if (e.key === 'Enter')
          onclick(row);
      });
    }
    if (selected && selected(row))
      tr.classList.add('selected');
    body.append(tr);
  }
  if (!rows.length)
    body.append(el('tr', {}, el('td', {colspan: columns.length, class: 'muted empty'}, empty)));
  return el('table', {class: 'data'}, el('thead', {}, head), body);
}

// localStorage that never throws
export const store = {
  get(key, fallback = null)
  {
    try
    {
      const value = localStorage.getItem('grid-console.' + key);
      return value === null ? fallback : JSON.parse(value);
    }
    catch (e)
    {
      return fallback;
    }
  },
  set(key, value)
  {
    try
    {
      localStorage.setItem('grid-console.' + key, JSON.stringify(value));
    }
    catch (e)
    {
      // Storage disabled: nothing is remembered
    }
  },
};
