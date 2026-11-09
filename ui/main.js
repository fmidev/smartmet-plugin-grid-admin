// grid-console: the page shell, the login state and the address routing.
//
// The state of the page is in the address (#/view?params), so every view can be
// bookmarked, opened in another tab and reached with the back button. The server only
// remembers the login.

import {api, setLoginHandler} from './api.js';
import {el, replace} from './dom.js';
import {renderLogin} from './login.js';
import {renderOverview} from './overview.js';
import {renderContent} from './content.js';
import {renderConfig} from './config.js';
import {renderLogs} from './logs.js';

const views = {
  overview: {label: 'Overview', render: renderOverview},
  content: {label: 'Content', render: renderContent},
  config: {label: 'Configuration', render: renderConfig},
  logs: {label: 'Logs', render: renderLogs},
};

const $ = (id) => document.getElementById(id);

let session = null;
let cleanup = null;

// ### The address

export function parseRoute()
{
  const hash = location.hash.replace(/^#\/?/, '');
  const [path, search = ''] = hash.split('?');
  const view = views[path] ? path : 'overview';
  const params = Object.fromEntries(new URLSearchParams(search));
  return {view, params};
}

export function routeHref(view, params = {})
{
  const q = new URLSearchParams();
  for (const [key, value] of Object.entries(params))
  {
    if (value !== undefined && value !== null && value !== '')
      q.set(key, value);
  }
  const s = q.toString();
  return '#/' + view + (s ? '?' + s : '');
}

// Moves to a view. replace = true changes the address without a history entry.
export function navigate(view, params = {}, {replace: replaceEntry = false} = {})
{
  const href = routeHref(view, params);
  if (href === location.hash)
    return;
  if (replaceEntry)
  {
    history.replaceState(null, '', href);
    render();
  }
  else
    location.hash = href;
}

// ### The page

function renderNav(active)
{
  if (active === null)
  {
    replace($('nav'));
    return;
  }
  replace($('nav'), Object.entries(views).map(([name, v]) =>
    el('a', {href: routeHref(name), class: name === active ? 'active' : '', 'aria-current': name === active ? 'page' : null}, v.label)));
}

function renderUser()
{
  const items = [];
  if (session?.legacyUrl)
    items.push(el('a', {href: session.legacyUrl, title: 'The earlier /grid-admin pages'}, 'Legacy UI'));
  if (session?.authenticationRequired && session?.user)
  {
    items.push(el('span', {class: 'muted', title: session.fullName || ''}, session.user));
    items.push(el('button', {type: 'button', class: 'link', onclick: logout}, 'Log out'));
  }
  replace($('user'), items);
}

function render()
{
  if (cleanup)
  {
    cleanup();
    cleanup = null;
  }

  const main = $('main');
  if (!session?.authenticated)
  {
    renderNav(null);
    renderLogin(main, (s) => {
      session = s;
      renderUser();
      render();
    });
    return;
  }

  const {view, params} = parseRoute();
  renderNav(view);
  document.title = `${views[view].label} – Grid console`;
  const root = el('div', {class: 'view'});
  replace(main, root);
  if (!session.canView)
  {
    replace(root, el('p', {class: 'error'}, 'Your account has no permission to view the grid engine information (grid-content-view).'));
    return;
  }
  cleanup = views[view].render(root, params, {session, navigate, routeHref, refresh: render}) || null;
}

async function logout()
{
  try
  {
    session = await api('logout', {}, {method: 'POST'});
  }
  catch (e)
  {
    session = await api('session');
  }
  renderUser();
  render();
}

async function start()
{
  setLoginHandler(() => {
    if (session?.authenticationRequired && session.authenticated)
    {
      // The session has expired on the server
      session.authenticated = false;
      session.user = null;
      renderUser();
      render();
    }
  });

  try
  {
    session = await api('session');
  }
  catch (e)
  {
    replace($('main'), el('p', {class: 'error'}, 'The server does not answer: ' + e.message));
    return;
  }
  renderUser();
  window.addEventListener('hashchange', render);
  render();
}

start();
