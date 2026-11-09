// The overview: the key figures of the grid engine, its content sources and the whole
// state information as a tree.

import {api, isAbort} from './api.js';
import {el, replace, table} from './dom.js';
import {formatBytes, formatCount, formatTime} from './format.js';

// The first node with the name anywhere below the node
function find(node, name)
{
  if (!node)
    return null;
  if (node.name === name)
    return node;
  for (const child of node.children || [])
  {
    const found = find(child, name);
    if (found)
      return found;
  }
  return null;
}

function value(node, ...names)
{
  for (const name of names)
  {
    node = (node?.children || []).find((c) => c.name === name);
    if (!node)
      return null;
  }
  return node.value ?? null;
}

function tile(label, text, detail = '')
{
  if (text === null || text === undefined || text === '')
    return null;
  return el('div', {class: 'tile'},
    el('div', {class: 'tile-label'}, label),
    el('div', {class: 'tile-value'}, text),
    detail ? el('div', {class: 'tile-detail muted'}, detail) : null);
}

function tiles(state)
{
  const records = find(state, 'Data Records');
  const swap = find(state, 'Content Swap');
  const gridCache = find(state, 'Grid Cache');
  const mapper = find(state, 'Memory mapper');

  const swapAgo = value(swap, 'Last Swap (seconds ago)');
  const swapInterval = value(swap, 'Swap Interval (seconds)');
  const cacheSize = value(gridCache, 'Current Size (Mb)');
  const cacheMax = value(gridCache, 'Max Size (Mb)');

  return el('div', {class: 'tiles'},
    tile('Producers', formatCount(value(records, 'Producer'))),
    tile('Generations', formatCount(value(records, 'Generation'))),
    tile('Files', formatCount(value(records, 'File'))),
    tile('Messages', formatCount(value(records, 'Content'))),
    tile('Geometries', formatCount(value(records, 'Geometry'))),
    swapAgo !== null ? tile('Content swap', `${swapAgo} s ago`, swapInterval ? `every ${swapInterval} s` : '') : null,
    cacheSize !== null ? tile('Grid cache', `${formatCount(cacheSize)} MB`, cacheMax ? `of ${formatCount(cacheMax)} MB` : '') : null,
    tile('Memory mapper', value(mapper, 'Enabled') === 'True' ? formatCount(value(mapper, 'Map requests processed')) : null, 'map requests'));
}

// The state tree: sections as <details>, values as rows
function tree(node, depth = 0)
{
  const leaves = (node.children || []).filter((c) => !c.children);
  const branches = (node.children || []).filter((c) => c.children);
  const body = [];
  if (leaves.length)
  {
    body.push(el('dl', {class: 'kv'}, leaves.map((c) => [
      el('dt', {}, c.name),
      el('dd', {class: c.value === '' ? 'muted' : ''}, c.value === '' ? '–' : c.value),
    ])));
  }
  for (const b of branches)
    body.push(tree(b, depth + 1));

  if (depth === 0)
    return el('div', {class: 'tree'}, body);
  return el('details', {class: 'tree-node', open: depth <= 1}, el('summary', {}, node.name), body);
}

// The installed SmartMet libraries, engines and plugins, read when the section is opened
function software()
{
  const body = el('div', {}, el('p', {class: 'muted'}, 'Loading…'));
  const details = el('details', {class: 'panel software'},
    el('summary', {}, el('h2', {class: 'inline'}, 'Installed SmartMet software')), body);
  let loaded = false;
  details.addEventListener('toggle', async () => {
    if (!details.open || loaded)
      return;
    loaded = true;
    try
    {
      const data = await api('software');
      replace(body, data.groups.map((g) => [
        el('h3', {}, `${g.title} (${g.directory})`),
        table([
          {label: 'File', class: 'mono', value: (f) => f.name},
          {label: 'Size', class: 'num', value: (f) => formatBytes(f.size)},
          {label: 'Modified', value: (f) => formatTime(f.modified)},
        ], g.files, {empty: 'None'}),
      ]));
    }
    catch (e)
    {
      loaded = false;
      replace(body, el('p', {class: 'error'}, e.message));
    }
  });
  return details;
}

export function renderOverview(root, params, {routeHref})
{
  const status = el('p', {class: 'muted'}, 'Loading…');
  replace(root, el('h1', {}, 'Grid engine'), status);

  let stopped = false;
  (async () => {
    try
    {
      const data = await api('overview', {}, {key: 'overview'});
      if (stopped)
        return;

      const sources = el('table', {class: 'data'},
        el('thead', {}, el('tr', {}, el('th', {}, 'Source'), el('th', {}, 'Type'), el('th', {}, 'Enabled'))),
        el('tbody', {}, data.sources.map((s) => el('tr', {},
          el('td', {}, el('a', {href: routeHref('content', {source: s.id})}, s.name)),
          el('td', {}, s.type),
          el('td', {}, s.enabled ? 'yes' : el('span', {class: 'warn'}, 'no'))))));

      replace(root,
        el('div', {class: 'heading'}, el('h1', {}, 'Grid engine'),
          el('span', {class: 'muted small'}, 'State at ' + formatTime(data.time) + ' UTC'),
          el('button', {type: 'button', onclick: () => renderOverview(root, params, {routeHref})}, 'Refresh')),
        tiles(data.state),
        el('section', {class: 'panel'}, el('h2', {}, 'Content sources'), sources,
          el('p', {class: 'muted small'}, 'The cache is the content information the grid engine uses; the sources are where it comes from.')),
        el('section', {class: 'panel'}, el('h2', {}, 'State information'), tree(data.state)),
        software());
    }
    catch (e)
    {
      if (!isAbort(e))
        replace(status, el('span', {class: 'error'}, e.message));
    }
  })();

  return () => {
    stopped = true;
  };
}
