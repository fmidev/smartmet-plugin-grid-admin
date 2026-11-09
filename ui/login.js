// The login form. The password is never sent: the server sends a one-time key, and the
// answer is SHA1(h + h + h + key) where h = SHA1("username:password"), as in the earlier
// login pages of /grid-admin.

import {api} from './api.js';
import {el, replace} from './dom.js';
import {sha1} from './sha1.js';

export function renderLogin(root, onSuccess)
{
  const user = el('input', {type: 'text', id: 'login-user', autocomplete: 'username', required: true});
  const password = el('input', {type: 'password', id: 'login-password', autocomplete: 'current-password', required: true});
  const message = el('p', {class: 'error', role: 'alert'});
  const button = el('button', {type: 'submit', class: 'primary'}, 'Log in');

  const form = el('form', {class: 'login panel'},
    el('h2', {}, 'Log in'),
    el('label', {for: 'login-user'}, 'Username'), user,
    el('label', {for: 'login-password'}, 'Password'), password,
    button, message);

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    button.disabled = true;
    message.textContent = '';
    try
    {
      const {key} = await api('login');
      const h = sha1(user.value + ':' + password.value);
      const session = await api('login', {}, {method: 'POST', body: user.value + ':' + sha1(h + h + h + key)});
      password.value = '';
      onSuccess(session);
    }
    catch (err)
    {
      message.textContent = err.message || 'Login failed';
      password.select();
    }
    finally
    {
      button.disabled = false;
    }
  });

  replace(root, el('div', {class: 'center'}, form));
  user.focus();
}
