// A text with line numbers and a filter that shows only the matching lines. Used for
// the configuration files and the logs.

import {el, replace, debounce} from './dom.js';

const MAX_LINES = 20000;

export function textView(lines, {placeholder = 'Filter lines', wrap = false, follow = false} = {})
{
  const input = el('input', {type: 'search', placeholder, 'aria-label': placeholder});
  const info = el('span', {class: 'muted small'});
  const pre = el('pre', {class: 'text' + (wrap ? ' wrap' : '')});

  let current = lines;

  const draw = () => {
    const words = input.value.trim().toLowerCase().split(/\s+/).filter(Boolean);
    const shown = [];
    for (let i = 0; i < current.length && shown.length < MAX_LINES; i++)
    {
      const line = current[i];
      if (!words.length || words.every((w) => line.toLowerCase().includes(w)))
        shown.push(el('span', {class: 'line', 'data-n': i + 1}, line + '\n'));
    }
    replace(pre, shown.length ? shown : el('span', {class: 'muted'}, words.length ? 'No line matches the filter' : '(empty)'));
    info.textContent = words.length ? `${shown.length} of ${current.length} lines` : `${current.length} lines` +
      (shown.length >= MAX_LINES ? `, the first ${MAX_LINES} shown` : '');
    if (follow)
      pre.scrollTop = pre.scrollHeight;
  };
  input.addEventListener('input', debounce(draw, 150));

  const node = el('div', {class: 'textview'}, el('div', {class: 'toolbar'}, input, info), pre);
  draw();

  return {
    node,
    setLines(newLines)
    {
      current = newLines;
      draw();
    },
    setWrap(on)
    {
      pre.classList.toggle('wrap', on);
    },
  };
}

export function splitLines(text)
{
  const lines = text.split('\n');
  if (lines.length && lines[lines.length - 1] === '')
    lines.pop();
  return lines;
}
