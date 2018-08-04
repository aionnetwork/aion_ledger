from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
from nose.tools import *
from crypto_helper import *

import nacl
from nacl.signing import SigningKey

import hashlib
import os
import struct

dongle = getDongle(True)

# 44'/425'
path = "44'/425'/0'/0'/0'"
bip44_path_default = (
               "8000002C"
              +"800001A9"
              +"80000000"
              +"80000000"
              +"80000000")


phrase = "pole museum pill kit hood museum style very mirror dash marble van satisfy payment ring unlock require peanut able symbol toy hint frame digital"

def parse_bip32_path(path):
    if len(path) == 0:
        return ""
    result = ""
    elements = path.split('/')
    for pathElement in elements:
        element = pathElement.split('\'')
        if len(element) == 1:
            result = result + struct.pack(">I", int(element[0]))            
        else:
            result = result + struct.pack(">I", 0x80000000 | int(element[0]))
    return result

def test_get_public_key_and_address():
    
    # get public key with bip path
    donglePath = parse_bip32_path(path)
    apdu = "e0020000".decode('hex') + chr(len(donglePath) + 1) + chr(len(donglePath) / 4) + donglePath
    pub_key = dongle.exchange(bytes(apdu))
    dongle_pub_key = str(pub_key).encode('hex')

    # generate an identical transaction
    seed = mnemonic_to_bip39seed(phrase, passphrase="")

    pk, cc = bip39_to_slip0010_master(seed)
    pk, cc = derive_child(pk, cc, 44)
    pk, cc = derive_child(pk, cc, 425)
    pk, cc = derive_child(pk, cc, 0)
    pk, cc = derive_child(pk, cc, 0)
    pk, cc = derive_child(pk, cc, 0)

    seed_pub_key, sec_key = nacl.bindings.crypto_sign_seed_keypair(pk)
    
    public_key = dongle_pub_key[0:64:1]
    address = dongle_pub_key[64:128:1]

    print "\nExpected public key : "+seed_pub_key.encode('hex')+"\n"
    print "Public key received from Ledger : "+public_key+"\n"
    print "Address received from Ledger : "+address+"\n"
    
    query = 'sh Map-Account-From-Public-Key.sh '+public_key
    f = os.popen(query) 
    query_result = f.readlines()
    actual_address = query_result[0].strip()

    assert seed_pub_key.encode('hex') == public_key
    assert actual_address == address

def test_verify_signature():

   # get public key with bip path
    donglePath = parse_bip32_path(path)
    apdu = "e0020000".decode('hex') + chr(len(donglePath) + 1) + chr(len(donglePath) / 4) + donglePath
    pub_key = dongle.exchange(bytes(apdu))
    dongle_pub_key = str(pub_key).encode('hex')

    # put transaction details
    nonce = '0'
    to_address = 'a0185ef98ac4841900b49ad9b432af2db7235e09ec3755e5ee36e9c4947007dd' # 32 byte hex string
    value = '100000000000000000000'   # big integer
    data = 'a0f2daa8de60d0e911fb468492242d60e15757408aff2902a0f2daa8de60d0e911fb468492242d604e1e11ec6f142bfee15757408aff2902a0f2daa8de60d0e911fb468492242d604e1e11ec6f142bfee15757408aff2902a0f2daa8de60d0e911fb468492242d604e1e11ec6f14a0f2daa0f2daa0f2daa0f2daa0f2' # hex string
    timestamp = '3287438' # big integer
    energy = '21000' # big integer
    energy_price = '10000000000' # long

    # generate rlp encode with contract data
    query = 'sh generate-rlp-encode.sh '+nonce+' '+energy_price+' '+energy+' '+to_address+' '+value+' '+data+' '+timestamp
    f = os.popen(query) 
    query_result = f.readlines()
    rlpEncode = query_result[0].strip()
    encodedTx = rlpEncode.decode('hex')
    print "\nEncoded tx to be sent : "+rlpEncode+"\n"

    # generate rlp encode without contract data
    # query = 'sh rlp-encode-without-contract-data.sh '+nonce+' '+energy_price+' '+energy+' '+to_address+' '+value+' '+timestamp
    # f = os.popen(query) 
    # query_result = f.readlines()
    # rlpEncode = query_result[0].strip()
    # encodedTx = rlpEncode.decode('hex')
    # print "\nEncoded tx to be sent : "+rlpEncode+"\n"

    # get blake2b hash
    query = 'sh Blake2b.sh '+rlpEncode
    f = os.popen(query) 
    query_result = f.readlines()
    blake2bHash = query_result[0].strip()
    print "\nExpected blake2b hash : "+blake2bHash+"\n"

    # get signature
    donglePath = parse_bip32_path(path)
    apdu = "e0040000".decode('hex') + chr(len(donglePath) + 1 + len(encodedTx)) + chr(len(donglePath) / 4) + bip44_path_default.decode('hex') + encodedTx
    signature = dongle.exchange(bytes(apdu))
    print "\nSignature received from ledger : "+str(signature).encode('hex')+"\n"

    # verify ledger signature
    query = 'sh validate-signature.sh '+blake2bHash+' '+str(signature).encode('hex')+' '+dongle_pub_key
    f = os.popen(query) 
    verification_result = f.readlines()
    assert "true" in str(verification_result)


