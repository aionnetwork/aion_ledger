from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
from nose.tools import *
from crypto_helper import *

import nacl
from nacl.signing import SigningKey

import hashlib
import os

dongle = getDongle(True)

# 44'/425'
bip44_path_default = (
               "8000002C"
              +"800001A9"
              +"80000000"
              +"80000000"
              +"80000000")

PATH_GET_PK = "8004"
PATH_SIGN_TX = "8002"

phrase = "pole museum pill kit hood museum style very mirror dash marble van satisfy payment ring unlock require peanut able symbol toy hint frame digital"

# verifies public key from ledger with expected public key from mnemonic bip39
def test_get_public_key():

    # get public key with bip path
    pub_key = dongle.exchange(bytes((PATH_GET_PK+bip44_path_default).decode('hex')))
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
    
    print "\nExpected public key : "+seed_pub_key.encode('hex')+"\n"
    print "Public key received from Ledger : "+dongle_pub_key+"\n"
    
    assert seed_pub_key.encode('hex') == dongle_pub_key

# verifies the signature received from ledger
def test_verify_signature():

    # get public key from ledger
    pub_key = dongle.exchange(bytes((PATH_GET_PK+bip44_path_default).decode('hex')))
    
    # generate random transaction hash
    tx_hash = hashlib.sha256()
    tx_hash.update(os.urandom(32))
    tx_hash = tx_hash.digest()
    print "\nTransaction Hash to be sent to Ledger : "+tx_hash.encode('hex')+"\n"

    # get signature from ledger
    signature = dongle.exchange(bytes((PATH_SIGN_TX+bip44_path_default+str(tx_hash).encode('hex')).decode('hex')))
    print "\nSignature received from Ledger : "+str(signature).encode('hex')+"\n"

    # verify ledger signature
    query = 'sh validate-signature.sh '+str(tx_hash).encode('hex')+' '+str(signature).encode('hex')+' '+str(pub_key).encode('hex')
    f = os.popen(query) 
    verificationResult = f.readlines()
    assert "true" in str(verificationResult)