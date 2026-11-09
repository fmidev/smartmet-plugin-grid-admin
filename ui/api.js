// The JSON API (/grid-console/api/NAME). The page is at /grid-console/, so the relative URLs work
// under any configured prefix.

export class ApiError extends Error
{
  constructor(message, status)
  {
    super(message);
    this.status = status;
  }
}

// Called when an answer says that a login is needed
let onLoginRequired = () => {};
export function setLoginHandler(f)
{
  onLoginRequired = f;
}

const controllers = new Map();

// Fetches api/NAME?params. A newer request with the same key aborts the older one.
export async function api(name, params = {}, {key = null, method = 'GET', body = null} = {})
{
  const url = 'api/' + name + query(params);

  let signal;
  if (key)
  {
    controllers.get(key)?.abort();
    const controller = new AbortController();
    controllers.set(key, controller);
    signal = controller.signal;
  }

  const options = {signal, method, cache: 'no-store'};
  if (method === 'POST')
  {
    // The server accepts a login or a logout only with this header
    options.headers = {'X-Grid-Console': '1', 'Content-Type': 'text/plain'};
    options.body = body ?? '';
  }

  const response = await fetch(url, options);
  let json = null;
  try
  {
    json = await response.json();
  }
  catch (e)
  {
    // Not JSON: an error page of the server
  }

  if (key && controllers.get(key)?.signal === signal)
    controllers.delete(key);

  if (!response.ok)
  {
    if (response.status === 401)
      onLoginRequired();
    throw new ApiError(json?.error || `${response.status} ${response.statusText}`, response.status);
  }
  return json;
}

export function query(params)
{
  const q = new URLSearchParams();
  for (const [key, value] of Object.entries(params))
  {
    if (value !== undefined && value !== null && value !== '')
      q.set(key, value);
  }
  const s = q.toString();
  return s ? '?' + s : '';
}

export function isAbort(e)
{
  return e && e.name === 'AbortError';
}
