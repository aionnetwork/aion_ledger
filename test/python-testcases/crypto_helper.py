import hmac
import hashlib

BIP39_PBKDF2_ROUNDS = 2048

HARDENED_KEY = 0x80000000
ED25519_HMAC_MODIFIER = b"ed25519 seed"
BIP39_SALT_MODIFIER = "mnemonic"

def mnemonic_to_bip39seed(mnemonic, passphrase):
    """ BIP39 seed from a mnemonic key.
        Logic adapted from https://github.com/trezor/python-mnemonic. """
    mnemonic = mnemonic
    salt = (BIP39_SALT_MODIFIER + passphrase).encode('utf-8')
    return hashlib.pbkdf2_hmac('sha512', mnemonic, salt, BIP39_PBKDF2_ROUNDS)

def bip39_to_slip0010_master(seed):
    h = hmac.new(ED25519_HMAC_MODIFIER, seed, hashlib.sha512).digest()
    # return key (k_{i}, k_{par}) 
    return h[:32], h[32:]

# paths are hardened by default
# warning: crappy code
def derive_child(priv_key, chain_code, offset):
    seed_concat = (
        b'\x00' +
        priv_key + 
        hex(HARDENED_KEY + offset)[2:].decode('hex'))
    h = hmac.new(chain_code, seed_concat, hashlib.sha512).digest()
    pk, cc = h[:32], h[32:]
    return pk, cc

def derive_path_child(mnemonic, path=[44, 425, 0, 0, 0]):
    seed = mnemonic_to_bip39seed(mnemonic, passphrase="")
