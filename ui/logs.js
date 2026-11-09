// The processing and debug logs of the content, data and query servers of the grid
// engine. Members of the grid-admin group can enable, disable and clear them when the
// engine configuration allows it (browser.flags).

import {api, isAbort} from './api.js';
import {el, replace, select, store} from './dom.js';
import {formatBytes, formatTime} from './format.js';
import {textView} from './textview.js';
import {confirmDialog, toast} from './form.js';

const serverNames = {content: 'Content server', data: 'Data server', query: 'Query server'};
const typeNames = {processing: 'Processing log', debug: 'Debug log'};
const REFRESH_MS = 5000;

export function renderLogs(root, params, ctx)
{
  const list = el('div', {class: 'list-body'}, el('p', {class: 'muted'}, 'Loading…'));
  const main = el('section', {class: 'logs-main'});
  replace(root, el('div', {class: 'split-layout'},
    el('aside', {class: 'list panel', 'aria-label': 'Logs'}, el('h2', {}, 'Logs'), list),
    main));

  let stopped = false;
  let timer = null;

  const server = params.server;
  const type = params.type;
  const lines = params.lines || '200';

  async function drawList()
  {
    const data = await api('logs', {}, {key: 'logs'});
    if (stopped)
      return;
    replace(list, Object.keys(serverNames).map((s) => [
      el('h3', {}, serverNames[s]),
      el('ul', {class: 'items'}, data.logs.filter((l) => l.server === s).map((l) => el('li', {},
        el('a', {
          href: ctx.routeHref('logs', {server: l.server, type: l.type}),
          class: l.server === server && l.type === type ? 'selected' : '',
          title: l.file || '',
        },
        el('span', {class: 'item-title'}, typeNames[l.type]),
        el('span', {class: 'item-detail muted'},
          !l.available ? 'not available' : (l.enabled ? 'enabled' : 'disabled') + (l.size !== undefined ? ' · ' + formatBytes(l.size) : '')))))),
    ]));
  }

  async function showLog()
  {
    if (!server || !type)
    {
      replace(main, el('h1', {}, 'Logs'), el('p', {class: 'muted'}, 'Select a log.'));
      return;
    }

    let view = null;
    const status = el('p', {class: 'muted small'});
    const header = el('h1', {}, `${serverNames[server] || server}: ${typeNames[type] || type}`);

    const follow = el('input', {type: 'checkbox', id: 'log-follow', checked: store.get('follow', false)});
    const wrap = el('input', {type: 'checkbox', id: 'log-wrap', checked: store.get('wrap', false)});
    const controls = el('div', {class: 'toolbar'},
      el('label', {}, 'Last ', select(['100', '200', '1000', '5000'], lines,
        (v) => ctx.navigate('logs', {server, type, lines: v}, {replace: true})), ' lines'),
      el('label', {for: 'log-follow'}, follow, ' Refresh every 5 s'),
      el('label', {for: 'log-wrap'}, wrap, ' Wrap lines'),
      el('button', {type: 'button', onclick: () => load()}, 'Refresh'),
      ctx.session.canControlLogs ? el('span', {class: 'actions'},
        el('button', {type: 'button', onclick: () => control('enable')}, 'Enable'),
        el('button', {type: 'button', onclick: () => control('disable')}, 'Disable'),
        el('button', {type: 'button', class: 'danger', onclick: async () => {
          if (await confirmDialog('Clear the log', `Clear the ${typeNames[type] || type} of the ${serverNames[server] || server}?`, {confirmLabel: 'Clear'}))
            control('clear');
        }}, 'Clear')) : null);

    async function control(action)
    {
      try
      {
        await api('logcontrol', {}, {method: 'POST', body: JSON.stringify({server, type, action})});
        toast(action === 'clear' ? 'The log was cleared.' : `The log was ${action}d.`);
        await drawList();
        await load();
      }
      catch (e)
      {
        toast(e.message, {error: true});
      }
    }

    const schedule = () => {
      clearTimeout(timer);
      if (follow.checked && !stopped)
        timer = setTimeout(load, REFRESH_MS);
    };
    follow.addEventListener('change', () => {
      store.set('follow', follow.checked);
      schedule();
    });
    wrap.addEventListener('change', () => {
      store.set('wrap', wrap.checked);
      view?.setWrap(wrap.checked);
    });

    replace(main, header, status, controls, el('p', {class: 'muted'}, 'Loading…'));

    async function load()
    {
      try
      {
        const data = await api('log', {server, type, lines}, {key: 'log'});
        if (stopped)
          return;
        status.textContent = !data.available ? 'The log is not available.' :
          `${data.file || 'no file'} · ${data.enabled ? 'enabled' : 'disabled'}` +
          (data.size !== undefined ? ` · ${formatBytes(data.size)} · modified ${formatTime(data.modified)} UTC` : '') +
          (!data.file ? '' : !data.exists ? ' · the file does not exist yet' :
            !data.regularFile ? ' · not a regular file, so it cannot be shown here' : '');
        if (!view)
        {
          view = textView(data.lines, {placeholder: 'Filter lines', wrap: wrap.checked, follow: true});
          main.lastChild.replaceWith(view.node);
        }
        else
          view.setLines(data.lines);
      }
      catch (e)
      {
        if (!isAbort(e) && !stopped)
          status.replaceChildren(el('span', {class: 'error'}, e.message));
      }
      schedule();
    }
    await load();
  }

  (async () => {
    try
    {
      await drawList();
      await showLog();
    }
    catch (e)
    {
      if (!isAbort(e) && !stopped)
        replace(main, el('p', {class: 'error'}, e.message));
    }
  })();

  return () => {
    stopped = true;
    clearTimeout(timer);
  };
}
