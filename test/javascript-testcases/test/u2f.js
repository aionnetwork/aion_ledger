import LedgerNode from '@ledgerhq/hw-transport-u2f';

const bipp44_path =
               '8000002C'
              + '800001A9'
              + '80000000'
              + '80000000'
              + '80000000';

describe('', () => {
  let dongle;

  before(async () => {
    const paths = await LedgerNode.list();
    console.log(paths);
    dongle = await LedgerNode.open(paths[0]);
    dongle.setScrambleKey('aion');
  });

  after(async () => {
    await dongle.close();
  });

  it('gets public key', async () => {
    const [publicKey] = await exchange(dongle, Buffer.from(`e00200001505${bipp44_path}`, 'hex'));
    console.log(`publicKey [${publicKey.length}]`, publicKey.toString('hex'));
  });

  it('gets signature', async () => {
    const [signature] = await exchange(dongle, Buffer.from(`e004000056058000002c800001a9800000008000000080000000f83f00a0a0185ef98ac4841900b49ad9b432af2db7235e09ec3755e5ee36e9c4947007dd89056bc75e2d6310000084aaaaaaaa8332298e8252088502540be40001`, 'hex'));
    console.log(`signature [${signature.length}]`, signature.toString('hex'));
  });

});

async function exchange(dongle, apdu) {
  console.log('HID =>', apdu.toString('hex'));
  const response = await dongle.exchange(apdu);
  console.log('HID <=', response.toString('hex'));
  return [response.slice(0, response.length - 2), response.slice(response.length - 2)];
}