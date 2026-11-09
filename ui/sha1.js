// SHA-1 of a UTF-8 string as hex. crypto.subtle would do this, but it exists only in
// secure contexts (HTTPS or localhost), and the admin pages are often used over plain HTTP.

export function sha1(text)
{
  const msg = new TextEncoder().encode(text);
  const H = [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0];

  const padded = new Uint8Array(((msg.length + 9 + 63) >> 6) << 6);
  padded.set(msg);
  padded[msg.length] = 0x80;
  const dv = new DataView(padded.buffer);
  // The bit length as a 64-bit big-endian number
  dv.setUint32(padded.length - 8, Math.floor(msg.length / 0x20000000), false);
  dv.setUint32(padded.length - 4, (msg.length * 8) >>> 0, false);

  const w = new Uint32Array(80);
  for (let i = 0; i < padded.length; i += 64)
  {
    for (let j = 0; j < 16; j++)
      w[j] = dv.getUint32(i + j * 4, false);
    for (let j = 16; j < 80; j++)
    {
      const x = w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16];
      w[j] = ((x << 1) | (x >>> 31)) >>> 0;
    }

    let [a, b, c, d, e] = H;
    for (let j = 0; j < 80; j++)
    {
      let f;
      let k;
      if (j < 20)
      {
        f = (b & c) | (~b & d);
        k = 0x5a827999;
      }
      else if (j < 40)
      {
        f = b ^ c ^ d;
        k = 0x6ed9eba1;
      }
      else if (j < 60)
      {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8f1bbcdc;
      }
      else
      {
        f = b ^ c ^ d;
        k = 0xca62c1d6;
      }
      const temp = (((a << 5) | (a >>> 27)) + f + e + k + w[j]) >>> 0;
      e = d;
      d = c;
      c = ((b << 30) | (b >>> 2)) >>> 0;
      b = a;
      a = temp;
    }
    H[0] = (H[0] + a) >>> 0;
    H[1] = (H[1] + b) >>> 0;
    H[2] = (H[2] + c) >>> 0;
    H[3] = (H[3] + d) >>> 0;
    H[4] = (H[4] + e) >>> 0;
  }
  return H.map((x) => x.toString(16).padStart(8, '0')).join('');
}
