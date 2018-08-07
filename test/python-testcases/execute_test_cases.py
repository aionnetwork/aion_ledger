from test_cases import *

# verifies public key from ledger with expected public key from mnemonic bip39
test_get_public_key_and_address();

# verifies the signature received from ledger
test_verify_signature_without_data();

# verifies the signature received from ledger
test_verify_signature_with_data();