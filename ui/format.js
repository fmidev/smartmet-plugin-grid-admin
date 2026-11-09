// Formatting of times, sizes and numbers

// "20261024T150000" -> Date (UTC)
export function parseTime(s)
{
  const m = /^(\d{4})(\d{2})(\d{2})T(\d{2})(\d{2})(\d{2})$/.exec(s || '');
  if (!m)
    return null;
  return new Date(Date.UTC(+m[1], +m[2] - 1, +m[3], +m[4], +m[5], +m[6]));
}

// "20261024T150000" -> "2026-10-24 15:00"
export function formatTime(s)
{
  const m = /^(\d{4})(\d{2})(\d{2})T(\d{2})(\d{2})(\d{2})$/.exec(s || '');
  if (!m)
    return s || '';
  return `${m[1]}-${m[2]}-${m[3]} ${m[4]}:${m[5]}` + (m[6] !== '00' ? ':' + m[6] : '');
}

// The age of a time relative to now: "35 min ago", "5 h ago", "in 2 h"
export function formatAge(s, now = Date.now())
{
  const d = parseTime(s);
  if (!d)
    return '';
  const minutes = Math.round((now - d.getTime()) / 60000);
  const future = minutes < 0;
  const m = Math.abs(minutes);
  let text;
  if (m < 1)
    return 'now';
  if (m < 90)
    text = `${m} min`;
  else if (m < 48 * 60)
    text = `${Math.round(m / 60)} h`;
  else
    text = `${Math.round(m / 1440)} d`;
  return future ? `in ${text}` : `${text} ago`;
}

// 1234567 -> "1.2 MB"
export function formatBytes(n)
{
  if (n === null || n === undefined || !Number.isFinite(Number(n)))
    return '';
  n = Number(n);
  const units = ['B', 'kB', 'MB', 'GB', 'TB'];
  let i = 0;
  while (n >= 1024 && i < units.length - 1)
  {
    n /= 1024;
    i++;
  }
  return (i === 0 ? String(n) : n.toFixed(n < 10 ? 1 : 0)) + ' ' + units[i];
}

// 26452064 -> "26 452 064"
export function formatCount(n)
{
  const s = String(n ?? '');
  return /^\d+$/.test(s) ? s.replace(/\B(?=(\d{3})+(?!\d))/g, ' ') : s;
}
