// The content browser: source › producer › generation › file › messages.
//
// The address holds the selection: #/content?source=cache&producer=1&generation=4&file=7.
// Files are paged by their ids (after = the first id of the page) and messages by their
// position (offset), because a generation can have hundreds of thousands of files.

import {api, isAbort} from './api.js';
import {el, replace, select, table, debounce} from './dom.js';
import {formatAge, formatBytes, formatCount, formatTime} from './format.js';
import {addRecord, updateRecord, deleteRecord} from './edit.js';

const FILE_PAGE = 100;
const CONTENT_PAGE = 500;

// The producer and generation lists are kept while only the deeper selection changes
const cache = {
  sources: null,
  producersKey: null,
  producers: null,
  generationsKey: null,
  generations: null,
};

// The ids where the earlier file pages started, for "Previous"
const filePages = {generation: null, stack: []};

let producerFilter = '';

function sourceName(id)
{
  return cache.sources?.find((s) => s.id === id)?.name || (id === 'cache' ? 'Grid engine cache' : 'Source ' + id);
}

function statusBadge(status)
{
  return el('span', {class: 'badge badge-' + status}, status);
}

function errorText(e)
{
  return el('p', {class: 'error'}, e.message);
}

function actions(...buttons)
{
  return el('div', {class: 'actions'}, buttons);
}

function button(label, onclick, cls = '')
{
  return el('button', {type: 'button', class: cls, onclick}, label);
}

// After a modification the lists are read again
function invalidate()
{
  cache.producersKey = null;
  cache.generationsKey = null;
}

export function renderContent(root, params, ctx)
{
  const source = params.source || 'cache';
  const producerId = params.producer || '';
  const generationId = params.generation || '';
  const fileId = params.file || '';

  const go = (changes, options) => ctx.navigate('content', Object.assign({}, params, changes), options);

  // Modifications are made to a content source, never to the cache of the engine
  const editable = ctx.session.canModify && source !== 'cache';
  const ids = (more = {}) => Object.assign({source}, more);
  const done = (result, changes = null) => {
    if (!result)
      return;
    invalidate();
    if (changes)
      go(changes);
    else
      ctx.refresh();
  };

  const producersPane = el('div', {class: 'list-body'}, el('p', {class: 'muted'}, 'Loading…'));
  const generationsPane = el('div', {class: 'list-body'});
  const main = el('section', {class: 'content-main'});

  const sourceSelect = el('div', {class: 'field'});
  const filter = el('input', {type: 'search', placeholder: 'Filter producers', value: producerFilter, 'aria-label': 'Filter producers'});

  replace(root,
    el('div', {class: 'content-layout'},
      el('aside', {class: 'list panel', 'aria-label': 'Producers'},
        el('div', {class: 'list-heading'}, el('h2', {}, 'Producers'),
          editable ? button('New', async () => {
            const r = await addRecord('producer', ids());
            done(r, r ? {producer: r.id, generation: null, file: null, after: null, offset: null} : null);
          }, 'small-button') : null),
        sourceSelect, filter,
        ctx.session.canModify && source === 'cache' ?
          el('p', {class: 'muted small'}, 'Choose a content source to modify the content information.') : null,
        !ctx.session.canModify && ctx.session.modifyNote ?
          el('p', {class: 'muted small'}, ctx.session.modifyNote) : null,
        producersPane),
      el('aside', {class: 'list panel', 'aria-label': 'Generations'},
        el('div', {class: 'list-heading'}, el('h2', {}, 'Generations'),
          editable && producerId ? button('New', async () => {
            const r = await addRecord('generation', ids({producer: producerId}), {status: 1});
            done(r, r ? {generation: r.id, file: null, after: null, offset: null} : null);
          }, 'small-button') : null),
        generationsPane),
      main));

  let stopped = false;
  const alive = () => !stopped;

  // ### Producers

  function drawProducers()
  {
    const f = producerFilter.trim().toLowerCase();
    const list = cache.producers.filter((p) => !f ||
      p.name.toLowerCase().includes(f) || p.title.toLowerCase().includes(f) || p.description.toLowerCase().includes(f));
    replace(producersPane, list.length ? el('ul', {class: 'items'}, list.map((p) =>
      el('li', {},
        el('a', {
          href: ctx.routeHref('content', {source, producer: p.id}),
          class: String(p.id) === producerId ? 'selected' : '',
          title: p.description,
        },
        el('span', {class: 'item-title'}, p.name),
        el('span', {class: 'item-detail muted'},
          `${p.generations} generation${p.generations === 1 ? '' : 's'}` + (p.newest ? ' · ' + formatTime(p.newest) : '')))))) :
      el('p', {class: 'muted'}, cache.producers.length ? 'No producer matches the filter' : 'No producers'));
  }

  filter.addEventListener('input', debounce(() => {
    producerFilter = filter.value;
    if (cache.producers)
      drawProducers();
  }, 150));

  async function loadProducers()
  {
    if (!cache.sources)
    {
      try
      {
        cache.sources = (await api('overview')).sources;
      }
      catch (e)
      {
        cache.sources = [{id: 'cache', name: 'Grid engine cache'}];
      }
    }
    if (!alive())
      return;
    replace(sourceSelect, el('label', {class: 'small muted'}, 'Source'),
      select(cache.sources.map((s) => ({value: s.id, label: s.name})), source,
        (value) => ctx.navigate('content', {source: value})));

    if (cache.producersKey !== source)
    {
      const data = await api('producers', {source}, {key: 'producers'});
      cache.producers = data.producers;
      cache.producersKey = source;
    }
    if (alive())
      drawProducers();
  }

  // ### Generations

  async function loadGenerations()
  {
    if (!producerId)
    {
      replace(generationsPane, el('p', {class: 'muted'}, 'Select a producer'));
      return null;
    }
    const key = source + '/' + producerId;
    if (cache.generationsKey !== key)
    {
      replace(generationsPane, el('p', {class: 'muted'}, 'Loading…'));
      const data = await api('generations', {source, producer: producerId}, {key: 'generations'});
      cache.generations = data;
      cache.generationsKey = key;
    }
    if (!alive())
      return null;

    const data = cache.generations;
    replace(generationsPane, data.generations.length ? el('ul', {class: 'items'}, data.generations.map((g) =>
      el('li', {},
        el('a', {
          href: ctx.routeHref('content', {source, producer: producerId, generation: g.id}),
          class: String(g.id) === generationId ? 'selected' : '',
          title: g.description || g.name,
        },
        el('span', {class: 'item-title'}, formatTime(g.analysisTime), ' ', statusBadge(g.status)),
        el('span', {class: 'item-detail muted'},
          g.name + (g.files !== undefined ? ` · ${formatCount(g.files)} file${g.files === 1 ? '' : 's'}` : '')))))) :
      el('p', {class: 'muted'}, 'No generations'));
    return data;
  }

  // ### Main area

  function breadcrumb(parts)
  {
    return el('nav', {class: 'breadcrumb', 'aria-label': 'Selection'}, parts.map((p, i) => [
      i ? el('span', {class: 'sep'}, '›') : null,
      p.href ? el('a', {href: p.href}, p.label) : el('span', {}, p.label),
    ]));
  }

  function producerSummary(producers)
  {
    const p = producers?.find((x) => String(x.id) === producerId);
    if (!p)
      return el('p', {class: 'muted'}, 'Select a producer and a generation.');
    return el('div', {},
      el('div', {class: 'heading'}, el('h1', {}, p.name),
        editable ? actions(
          button('Edit', async () => done(await updateRecord('producer', ids({producer: p.id}), p))),
          button('Delete', async () => done(await deleteRecord('producer', ids({producer: p.id}), `the producer ${p.name}`),
            {producer: null, generation: null, file: null}), 'danger')) : null),
      el('dl', {class: 'kv'},
        el('dt', {}, 'Title'), el('dd', {}, p.title),
        el('dt', {}, 'Description'), el('dd', {}, p.description),
        el('dt', {}, 'Id'), el('dd', {}, p.id),
        el('dt', {}, 'Source id'), el('dd', {}, p.sourceId),
        el('dt', {}, 'Flags'), el('dd', {}, p.flags),
        el('dt', {}, 'Generations'), el('dd', {}, p.generations)),
      el('p', {class: 'muted'}, 'Select a generation to see its files.'));
  }

  async function showFiles()
  {
    const after = Number(params.after || 0);
    if (filePages.generation !== source + '/' + generationId)
    {
      filePages.generation = source + '/' + generationId;
      filePages.stack = [];
    }

    replace(main, el('p', {class: 'muted'}, 'Loading…'));
    const data = await api('files', {source, generation: generationId, after: after || null, limit: FILE_PAGE}, {key: 'main'});
    if (!alive())
      return;

    const g = data.generation;
    const makePager = () => el('div', {class: 'pager'},
      el('button', {type: 'button', disabled: !after, onclick: () => {
        filePages.stack = [];
        go({after: null, file: null});
      }}, 'First'),
      el('button', {type: 'button', disabled: !filePages.stack.length, onclick: () => {
        go({after: filePages.stack.pop() || null, file: null});
      }}, 'Previous'),
      el('button', {type: 'button', disabled: data.next === null, onclick: () => {
        filePages.stack.push(after);
        go({after: data.next, file: null});
      }}, 'Next'),
      el('span', {class: 'muted small'},
        `${data.files.length} file${data.files.length === 1 ? '' : 's'} from id ${after || data.files[0]?.id || 0}` +
        (data.count !== undefined ? ` · ${formatCount(data.count)} in total` : '')));

    replace(main,
      breadcrumb([
        {label: sourceName(source), href: ctx.routeHref('content', {source})},
        {label: data.producer.name, href: ctx.routeHref('content', {source, producer: producerId})},
        {label: formatTime(g.analysisTime)},
      ]),
      el('div', {class: 'heading'}, el('h1', {}, `${data.producer.name} ${formatTime(g.analysisTime)}`), statusBadge(g.status),
        editable ? actions(
          button('Edit', async () => done(await updateRecord('generation', ids({generation: g.id}),
            Object.assign({}, g, {status: g.statusCode})))),
          button('Delete', async () => done(await deleteRecord('generation', ids({generation: g.id}), `the generation ${g.name}`),
            {generation: null, file: null, after: null}), 'danger'),
          button('New file', async () => {
            const r = await addRecord('file', ids({generation: g.id}), {typeCode: 2, serverTypeCode: 1, protocolCode: 0});
            done(r, r ? {file: r.id, offset: null} : null);
          })) : null),
      el('dl', {class: 'kv kv-wide'},
        el('dt', {}, 'Name'), el('dd', {}, g.name),
        el('dt', {}, 'Description'), el('dd', {}, g.description || '–'),
        el('dt', {}, 'Id'), el('dd', {}, g.id),
        el('dt', {}, 'Valid times'), el('dd', {}, g.start ? `${formatTime(g.start)} … ${formatTime(g.end)}` : '–'),
        el('dt', {}, 'Modified'), el('dd', {}, g.modified ? `${formatTime(g.modified)} (${formatAge(g.modified)})` : '–'),
        el('dt', {}, 'Deletion'), el('dd', {}, g.deletion ? `${formatTime(g.deletion)} (${formatAge(g.deletion)})` : 'never')),
      makePager(),
      table([
        {label: 'Id', class: 'num', value: (f) => f.id},
        {label: 'File', class: 'mono wrap', value: (f) => el('a', {href: ctx.routeHref('content', Object.assign({}, params, {file: f.id, offset: null}))}, f.name)},
        {label: 'Type', value: (f) => f.type},
        {label: 'Server', value: (f) => [f.serverType, f.protocol, f.server].filter(Boolean).join(' ')},
        {label: 'Modified', value: (f) => formatTime(f.modified)},
        {label: 'Deletion', value: (f) => f.deletion ? formatTime(f.deletion) : ''},
      ], data.files, {
        onclick: (f) => go({file: f.id, offset: null}),
        empty: 'No files',
      }),
      data.files.length > 20 ? makePager() : null);
  }

  function viewerLink(data, c)
  {
    if (!ctx.session.viewerUrl || !c.parameter)
      return null;
    const q = new URLSearchParams({
      producer: data.producer.name,
      generation: data.generation.analysisTime,
      parameter: c.parameter,
      levelType: c.levelId,
      level: c.level,
      forecastType: c.forecastType,
      member: c.forecastNumber,
      geometry: c.geometry,
      time: c.time,
    });
    return el('a', {href: `${ctx.session.viewerUrl}?${q}`, target: '_blank', rel: 'noopener', title: 'Show in the grid-gui viewer'}, 'View');
  }

  async function showContents()
  {
    const offset = Number(params.offset || 0);
    replace(main, el('p', {class: 'muted'}, 'Loading…'));
    const data = await api('contents', {source, file: fileId, offset: offset || null, limit: CONTENT_PAGE}, {key: 'main'});
    if (!alive())
      return;

    const f = data.file;
    const g = data.generation;
    const filterInput = el('input', {type: 'search', placeholder: 'Filter messages (parameter, level, time…)', 'aria-label': 'Filter messages'});
    const tableHolder = el('div', {class: 'table-scroll'});

    const columns = [
      {label: 'Msg', class: 'num', value: (c) => c.message},
      {label: 'Parameter', class: 'mono', value: (c) => c.parameter},
      {label: 'Level type', value: (c) => c.levelName ? `${c.levelName} (${c.levelId})` : c.levelId},
      {label: 'Level', class: 'num', value: (c) => c.level},
      {label: 'Time', value: (c) => formatTime(c.time)},
      {label: 'Type', class: 'num', title: 'Forecast type', value: (c) => c.forecastType},
      {label: 'Member', class: 'num', title: 'Forecast number', value: (c) => c.forecastNumber},
      {label: 'Geometry', value: (c) => el('span', {title: c.geometryName}, c.geometry)},
      {label: 'Size', class: 'num', value: (c) => formatBytes(c.size)},
      {label: '', value: (c) => viewerLink(data, c)},
    ];
    if (editable)
    {
      columns.push({label: '', value: (c) => el('span', {class: 'row-actions'},
        button('Edit', async () => done(await updateRecord('content', ids({file: f.id, message: c.message}), c)), 'small-button'),
        button('Delete', async () => done(await deleteRecord('content', ids({file: f.id, message: c.message}),
          `message ${c.message} (${c.parameter} ${formatTime(c.time)})`)), 'small-button danger'))});
    }

    const draw = () => {
      const words = filterInput.value.trim().toLowerCase().split(/\s+/).filter(Boolean);
      const rows = words.length ? data.contents.filter((c) => {
        const text = [c.parameter, c.levelName, c.levelId, c.level, c.time, formatTime(c.time), c.geometry, c.geometryName, c.forecastType, c.forecastNumber].join(' ').toLowerCase();
        return words.every((w) => text.includes(w));
      }) : data.contents;
      replace(tableHolder, table(columns, rows, {empty: words.length ? 'No message matches the filter' : 'No messages'}));
    };
    filterInput.addEventListener('input', debounce(draw, 150));

    const last = offset + data.contents.length;
    const pager = data.count > CONTENT_PAGE ? el('div', {class: 'pager'},
      el('button', {type: 'button', disabled: offset === 0, onclick: () => go({offset: Math.max(0, offset - CONTENT_PAGE) || null})}, 'Previous'),
      el('button', {type: 'button', disabled: last >= data.count, onclick: () => go({offset: last})}, 'Next'),
      el('span', {class: 'muted small'}, `Messages ${offset + 1}–${last} of ${formatCount(data.count)}`)) :
      el('p', {class: 'muted small'}, `${formatCount(data.count)} message${data.count === 1 ? '' : 's'}`);

    replace(main,
      breadcrumb([
        {label: sourceName(source), href: ctx.routeHref('content', {source})},
        {label: data.producer.name, href: ctx.routeHref('content', {source, producer: producerId})},
        {label: formatTime(g.analysisTime), href: ctx.routeHref('content', {source, producer: producerId, generation: generationId, after: params.after})},
        {label: 'File ' + f.id},
      ]),
      el('div', {class: 'heading'}, el('h1', {class: 'mono wrap'}, f.name),
        editable ? actions(
          button('Edit', async () => done(await updateRecord('file', ids({file: f.id}), f))),
          button('Delete', async () => done(await deleteRecord('file', ids({file: f.id}), `the file ${f.name}`),
            {file: null, offset: null}), 'danger'),
          button('New message', async () => {
            const last = data.count > 0 ? data.contents[data.contents.length - 1] : null;
            const defaults = last ? Object.assign({}, last, {message: last.message + 1, position: '', size: ''}) : {message: 0};
            done(await addRecord('content', ids({file: f.id}), defaults));
          })) : null),
      el('dl', {class: 'kv kv-wide'},
        el('dt', {}, 'Id'), el('dd', {}, f.id),
        el('dt', {}, 'Type'), el('dd', {}, f.type),
        el('dt', {}, 'Server'), el('dd', {}, [f.serverType, f.protocol, f.server].filter(Boolean).join(' ') || 'local'),
        el('dt', {}, 'Source id'), el('dd', {}, f.sourceId),
        el('dt', {}, 'Flags'), el('dd', {}, f.flags),
        el('dt', {}, 'Modified'), el('dd', {}, f.modified ? `${formatTime(f.modified)} (${formatAge(f.modified)})` : '–'),
        el('dt', {}, 'Deletion'), el('dd', {}, f.deletion ? `${formatTime(f.deletion)} (${formatAge(f.deletion)})` : 'never')),
      el('div', {class: 'toolbar'}, filterInput, pager),
      tableHolder);
    draw();
  }

  // ### Loading in order: the lists first, then the main area

  (async () => {
    try
    {
      await loadProducers();
      if (!alive())
        return;
      await loadGenerations();
      if (!alive())
        return;
      if (fileId)
        await showContents();
      else if (generationId)
        await showFiles();
      else
        replace(main, producerSummary(cache.producers));
    }
    catch (e)
    {
      if (!isAbort(e) && alive())
        replace(main, errorText(e));
    }
  })();

  // Scrolls the selected list items into view once they exist
  requestAnimationFrame(() => setTimeout(() => {
    root.querySelectorAll('.items a.selected').forEach((a) => a.scrollIntoView({block: 'nearest'}));
  }, 300));

  return () => {
    stopped = true;
  };
}
