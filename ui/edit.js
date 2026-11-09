// Modifying the content information: the forms of producers, generations, files and
// messages, and the calls of api/modify. The server checks the permissions and the
// values again; these forms only help to fill them in.

import {api} from './api.js';
import {editDialog, confirmDialog, toast} from './form.js';

const fileTypes = [
  {value: 0, label: 'Unknown'}, {value: 1, label: 'GRIB1'}, {value: 2, label: 'GRIB2'},
  {value: 3, label: 'NetCDF3'}, {value: 4, label: 'NetCDF4'}, {value: 5, label: 'QueryData'},
  {value: 6, label: 'GeoTIFF'}, {value: 9, label: 'GRIB (mixed)'},
];
const serverTypes = [
  {value: 0, label: 'Unknown'}, {value: 1, label: 'File system'}, {value: 2, label: 'S3'},
  {value: 3, label: 'THREDDS'}, {value: 4, label: 'HTTPD'},
];
const protocols = [{value: 0, label: 'None'}, {value: 1, label: 'HTTP'}, {value: 2, label: 'HTTPS'}];
const statuses = [{value: 0, label: 'Disabled'}, {value: 1, label: 'Ready'}, {value: 2, label: 'Running'}];

const forms = {
  producer: [
    {name: 'name', label: 'Name', type: 'text', required: true},
    {name: 'title', label: 'Title', type: 'text'},
    {name: 'description', label: 'Description', type: 'text'},
    {name: 'flags', label: 'Flags', type: 'int'},
    {name: 'sourceId', label: 'Source id', type: 'int'},
  ],
  generation: [
    {name: 'name', label: 'Name', type: 'text', required: true, help: 'Usually producer:analysis time'},
    {name: 'analysisTime', label: 'Analysis time', type: 'time', required: true},
    {name: 'description', label: 'Description', type: 'text'},
    {name: 'status', label: 'Status', type: 'select', options: statuses},
    {name: 'flags', label: 'Flags', type: 'int'},
    {name: 'sourceId', label: 'Source id', type: 'int'},
    {name: 'deletion', label: 'Deletion time', type: 'time', help: 'Empty: never deleted'},
  ],
  file: [
    {name: 'name', label: 'Name', type: 'text', required: true, help: 'The path or the object name'},
    {name: 'typeCode', label: 'Type', type: 'select', options: fileTypes},
    {name: 'serverTypeCode', label: 'Server type', type: 'select', options: serverTypes},
    {name: 'protocolCode', label: 'Protocol', type: 'select', options: protocols},
    {name: 'server', label: 'Server', type: 'text'},
    {name: 'flags', label: 'Flags', type: 'int'},
    {name: 'sourceId', label: 'Source id', type: 'int'},
    {name: 'deletion', label: 'Deletion time', type: 'time', help: 'Empty: never deleted'},
  ],
  content: [
    {name: 'parameter', label: 'Parameter', type: 'text', required: true, help: 'FMI name, for example T-K'},
    {name: 'fmiParameterId', label: 'FMI parameter id', type: 'int'},
    {name: 'levelId', label: 'Level type', type: 'int'},
    {name: 'level', label: 'Level', type: 'int'},
    {name: 'time', label: 'Forecast time', type: 'time', required: true},
    {name: 'forecastType', label: 'Forecast type', type: 'int'},
    {name: 'forecastNumber', label: 'Forecast number', type: 'int'},
    {name: 'geometry', label: 'Geometry', type: 'int'},
    {name: 'position', label: 'File position', type: 'int'},
    {name: 'size', label: 'Message size', type: 'int'},
    {name: 'aggregationId', label: 'Aggregation id', type: 'int'},
    {name: 'aggregationPeriod', label: 'Aggregation period', type: 'int'},
    {name: 'processingTypeId', label: 'Processing type', type: 'int'},
    {name: 'processingTypeValue1', label: 'Processing value 1', type: 'number'},
    {name: 'processingTypeValue2', label: 'Processing value 2', type: 'number'},
    {name: 'flags', label: 'Flags', type: 'int'},
    {name: 'sourceId', label: 'Source id', type: 'int'},
    {name: 'deletion', label: 'Deletion time', type: 'time', help: 'Empty: never deleted'},
  ],
};

const typeNames = {producer: 'producer', generation: 'generation', file: 'file', content: 'message'};

const NOTE = 'The change is made to the content source. The grid engine sees it after its next content update.';

async function send(body)
{
  try
  {
    const result = await api('modify', {}, {method: 'POST', body: JSON.stringify(body)});
    toast(`The ${typeNames[body.type]} was ${body.action === 'add' ? 'added' : body.action === 'update' ? 'updated' : 'deleted'}.`);
    return result;
  }
  catch (e)
  {
    toast(e.message, {error: true});
    return null;
  }
}

// Only the changed fields are sent with an update
function changed(values, record)
{
  const result = {};
  for (const [name, value] of Object.entries(values))
  {
    if (!record || String(record[name] ?? '') !== value)
      result[name] = value;
  }
  return result;
}

// ids: {source, producer, generation, file, message} as needed. Resolves to the result or null.
export async function addRecord(type, ids, defaults = {})
{
  const values = await editDialog(`New ${typeNames[type]}`,
    type === 'content' ? [{name: 'message', label: 'Message index', type: 'int', required: true}, ...forms[type]] : forms[type],
    defaults, {submitLabel: 'Add', note: NOTE});
  if (!values)
    return null;
  const body = Object.assign({type, action: 'add'}, ids);
  if (type === 'content')
  {
    body.message = values.message;
    delete values.message;
  }
  // Empty optional numbers are left to their defaults
  body.fields = Object.fromEntries(Object.entries(values).filter(([, v]) => v !== ''));
  return send(body);
}

export async function updateRecord(type, ids, record)
{
  const values = await editDialog(`Edit ${typeNames[type]}`, forms[type], record, {note: NOTE});
  if (!values)
    return null;
  const fields = changed(values, record);
  if (!Object.keys(fields).length)
  {
    toast('Nothing was changed.');
    return null;
  }
  return send(Object.assign({type, action: 'update', fields}, ids));
}

export async function deleteRecord(type, ids, description)
{
  const extra = type === 'producer' ? ' Its generations, files and messages are deleted too.' :
    type === 'generation' ? ' Its files and messages are deleted too.' :
    type === 'file' ? ' Its messages are deleted too.' : '';
  if (!await confirmDialog(`Delete ${typeNames[type]}`, `Delete ${description}?${extra} This cannot be undone.`))
    return null;
  return send(Object.assign({type, action: 'delete'}, ids));
}
