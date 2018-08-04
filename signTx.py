#!/usr/bin/env python
"""
*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************
"""
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
import argparse
import struct
from decimal import Decimal
from ethBase import Transaction, UnsignedTransaction
from rlp import encode
from rlp.utils import decode_hex, encode_hex, str_to_bytes

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

encodedTx = 'f8e400a0a0f2daa8de60d0e911fb468492242d604e1e11ec6f142bfee15757408aff29028a021e19e0c9bab2400000b8a4a0f2daa8de60d0e911fb468492242d604e1e11ec6f142bfee15757408aff2902a0f2daa8de60d0e911fb468492242d604e1e11ec6f142bfee15757408aff2902a0f2daa8de60d0e911fb468492242d604e1e11ec6f142bfee15757408aff2902a0f2daa8de60d0e911fb468492242d604e1e11ec6f142bfee15757408aff2902a0f2daa8de60d0e911fb468492242d604e1e11ec6f14a0f2daa0f2daa0f2daa0f2daa0f28601650162f0908259d88502540be40001'.decode('hex');
path = "44'/425'/0'/0'/0'"

donglePath = parse_bip32_path(path)
apdu = "e0040000".decode('hex') + chr(len(donglePath) + 1 + len(encodedTx)) + chr(len(donglePath) / 4) + donglePath + encodedTx

dongle = getDongle(True)
result = dongle.exchange(bytes(apdu))



print "Signed transaction " + str(result).encode('hex')
