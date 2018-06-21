from test_cases import *

# verifies public key from ledger with expected public key from mnemonic bip39
test_get_public_key();

# verifies the signature received from ledger
test_verify_signature();