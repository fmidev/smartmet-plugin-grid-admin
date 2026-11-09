// The configuration files of the grid engine. A file is chosen by its kind and index
// in the engine configuration, so only the configured files can be read.

import {api, isAbort} from './api.js';
import {el, replace} from './dom.js';
import {formatBytes, formatTime} from './format.js';
import {textView, splitLines} from './textview.js';

let listCache = null;

export function renderConfig(root, params, ctx)
{
  const list = el('div', {class: 'list-body'}, el('p', {class: 'muted'}, 'Loading…'));
  const main = el('section', {class: 'config-main'});
  replace(root, el('div', {class: 'split-layout'},
    el('aside', {class: 'list panel', 'aria-label': 'Files'}, el('h2', {}, 'Files'), list),
    main));

  let stopped = false;

  const isSelected = (f) => f.kind === params.kind && String(f.index) === String(params.index ?? 0);
  const fileName = (path) => path.split('/').pop();

  (async () => {
    try
    {
      if (!listCache)
        listCache = await api('config');
      if (stopped)
        return;

      replace(list, listCache.groups.map((g) => [
        el('h3', {}, g.title),
        g.files.length ? el('ul', {class: 'items'}, g.files.map((f) => el('li', {},
          el('a', {
            href: ctx.routeHref('config', {kind: f.kind, index: f.index}),
            class: (isSelected(f) ? 'selected ' : '') + (f.exists ? '' : 'missing'),
            title: f.path,
          },
          el('span', {class: 'item-title'}, f.kind === 'engine' ? 'Engine configuration' : f.kind === 'producers' ? 'Producer search file' : fileName(f.path)),
          el('span', {class: 'item-detail muted'}, f.exists ? formatBytes(f.size) : 'missing'))))) :
          el('p', {class: 'muted small'}, 'None'),
      ]));

      if (!params.kind)
      {
        replace(main, el('h1', {}, 'Configuration'),
          el('p', {class: 'muted'}, 'Select a file. Passwords in the engine configuration are hidden.'),
          el('button', {type: 'button', onclick: () => {
            listCache = null;
            renderConfig(root, params, ctx);
          }}, 'Reload the list'));
        return;
      }

      replace(main, el('p', {class: 'muted'}, 'Loading…'));
      const file = await api('configfile', {kind: params.kind, index: params.index ?? 0}, {key: 'configfile'});
      if (stopped)
        return;

      const view = textView(splitLines(file.text));
      replace(main,
        el('h1', {class: 'mono wrap'}, file.path),
        el('p', {class: 'muted small'}, `${formatBytes(file.size)} · modified ${formatTime(file.modified)} UTC`),
        file.redacted ? el('p', {class: 'warn small'}, `${file.redacted} line${file.redacted === 1 ? '' : 's'} mentioning a password hidden.`) : null,
        file.truncated ? el('p', {class: 'warn small'}, 'The file is long: only its beginning is shown.') : null,
        view.node);
    }
    catch (e)
    {
      if (!isAbort(e) && !stopped)
        replace(main, el('p', {class: 'error'}, e.message));
    }
  })();

  return () => {
    stopped = true;
  };
}
