/*
 * Copyright (c) 2017-2018 Aion foundation.
 *
 *     This file is part of the aion network project.
 *
 *     The aion network project is free software: you can redistribute it 
 *     and/or modify it under the terms of the GNU General Public License 
 *     as published by the Free Software Foundation, either version 3 of 
 *     the License, or any later version.
 *
 *     The aion network project is distributed in the hope that it will 
 *     be useful, but WITHOUT ANY WARRANTY; without even the implied 
 *     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *     See the GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with the aion network project source files.  
 *     If not, see <https://www.gnu.org/licenses/>.
 *
 *     The aion network project leverages useful source code from other 
 *     open source projects. We greatly appreciate the effort that was 
 *     invested in these projects and we thank the individual contributors 
 *     for their work. For provenance information and contributors
 *     please see <https://github.com/aionnetwork/aion/wiki/Contributors>.
 *
 * Contributors to the aion source files in decreasing order of code volume:
 *     Aion foundation.
 *     <ether.camp> team through the ethereumJ library.
 *     Ether.Camp Inc. (US) team through Ethereum Harmony.
 *     John Tromp through the Equihash solver.
 *     Samuel Neves through the BLAKE2 implementation.
 *     Zcash project team.
 *     Bitcoinj team.
 */
#include "os.h"
#include "cx.h"

#include "os_io_seproxyhal.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

// UI currently displayed
enum UI_STATE { UI_IDLE, UI_TEXT };

enum UI_STATE uiState;

ux_state_t ux;
bagl_element_t tmp_element;

static const bagl_element_t *io_seproxyhal_touch_exit(const bagl_element_t *e);
static const bagl_element_t*
io_seproxyhal_touch_approve(const bagl_element_t *e);
static const bagl_element_t *io_seproxyhal_touch_deny(const bagl_element_t *e);

static void ui_idle(void);
static void populate_transaction(void);
static void show_transaction(void);

#define DEFAULT_FONT BAGL_FONT_OPEN_SANS_LIGHT_16px | BAGL_FONT_ALIGNMENT_LEFT
#define CLA 0x80
#define INS_SIGN 0x02
#define INS_GET_PUBLIC_KEY 0x04
#define MEMCLEAR(dest) { os_memset(&dest, 0, sizeof(dest)); }
#define BIP_PATH_END 22
#define TRANSACTION_HASH_SIZE 32
#define KEY_SIZE 32
#define SIGNATURE_SIZE 64

static char transactionDetail[2*TRANSACTION_HASH_SIZE+1];

// contents of screen in idle state
static const bagl_element_t bagl_ui_idle_nanos[] = {
    {
        {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000,
         0xFFFFFF, 0, 0},
        NULL,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    {
        {BAGL_LABELINE, 0x02, 0, 11, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000,
         BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        "Waiting For",
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    {
        {BAGL_LABELINE, 0x02, 0, 25, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000,
         BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        "Aion Transaction",
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    {
        {BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
         BAGL_GLYPH_ICON_CROSS},
        NULL,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
};

// button events for idle screen
static unsigned int
bagl_ui_idle_nanos_button(unsigned int button_mask,
                          unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
    case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        io_seproxyhal_touch_exit(NULL);
        break;
    }

    return 0;
}

// screen content when transaction is received by ledger
static const bagl_element_t bagl_ui_text_review_nanos[] = {
    {
        {BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000,
         0xFFFFFF, 0, 0},
        NULL,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    {
        {BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
         BAGL_GLYPH_ICON_CROSS},
        NULL,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    {
        {BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0,
         BAGL_GLYPH_ICON_CHECK},
        NULL,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    {
        {BAGL_LABELINE, 0x01, 0, 12, 128, 11, 0, 0, 0, 0xFFFFFF, 0x000000,
         BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
        "Sign Aion Tx",
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    {
        {BAGL_LABELINE, 0x02, 23, 26, 82, 11, 0x80 | 10, 0, 0, 0xFFFFFF,
         0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
    },
    
};

// button events for sign event screen
static unsigned int
bagl_ui_text_review_nanos_button(unsigned int button_mask,
                                 unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        io_seproxyhal_touch_approve(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_deny(NULL);
        break;
    }
    // clear transaction detail in case of accept/reject button event
    MEMCLEAR(transactionDetail);
    return 0;
}

static const bagl_element_t *io_seproxyhal_touch_exit(const bagl_element_t *e) {
    // Go back to the dashboard
    os_sched_exit(0);
    return NULL; // do not redraw the widget
}

static const bagl_element_t *io_seproxyhal_touch_deny(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;

    // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0; 
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                          sizeof(G_io_apdu_buffer), 0);
        }

    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

// contents for idle screen
static void ui_idle(void) {
    uiState = UI_IDLE;
    UX_DISPLAY(bagl_ui_idle_nanos, NULL);
}

// text motion if string is too long for screen
const bagl_element_t *ui_transaction_prepro(const bagl_element_t *element) {
    if (element->component.userid == 0x02) {
        os_memmove(&tmp_element, element, sizeof(bagl_element_t));
        tmp_element.text = transactionDetail;
        UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
        return &tmp_element;
    }
    return element;
}

// contents for sign screen
static void show_transaction(void) {
    uiState = UI_TEXT;
    UX_DISPLAY(bagl_ui_text_review_nanos, ui_transaction_prepro);
}

unsigned char io_event(unsigned char channel) {

    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT: // for Nano S
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        if ((uiState == UI_TEXT) &&
            (os_seph_features() &
             SEPROXYHAL_TAG_SESSION_START_EVENT_FEATURE_SCREEN_BIG)) {
                show_transaction();
            
        } else {
            UX_DISPLAYED_EVENT();
        }
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
            UX_CALLBACK_SET_INTERVAL(500);
            UX_REDISPLAY();
        });
        break;

    default:
        UX_DEFAULT_EVENT();
        break;
    }

    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    return 1;
}

// convert digit to hex
char get_hex(int val){
    switch(val){
        case 0:
            return '0';
        case 1:
            return '1';
        case 2:
            return '2';
        case 3:
            return '3';
        case 4:
            return '4';
        case 5:
            return '5';
        case 6:
            return '6';
        case 7:
            return '7';
        case 8:
            return '8';
        case 9:
            return '9';
        case 10:
            return 'a';
        case 11:
            return 'b';
        case 12:
            return 'c';
        case 13:
            return 'd';
        case 14:
            return 'e';
        case 15:
            return 'f';
        default :
            return -1;
    }
}

// convert byte to int
int atoh (char data, int is_msb) {
    return is_msb==0 ? data&0x0f : ((data&0xf0)>>4);
}

// populate transaction detail buffer
static void populate_transaction() {
    unsigned int counter = 0;
    while (counter<TRANSACTION_HASH_SIZE) {
        transactionDetail[counter*2]= get_hex(atoh(G_io_apdu_buffer[BIP_PATH_END+counter],1));
        transactionDetail[counter*2+1]= get_hex(atoh(G_io_apdu_buffer[BIP_PATH_END+counter],0));
        counter++;
    }
    transactionDetail[2*TRANSACTION_HASH_SIZE] = '\0';
}

// fetch public key from cx_ecfp_public_key_t struct
void extract_public_key(cx_ecfp_public_key_t* pubKey)
{
    for (int i = 0; i < KEY_SIZE; i++) {
        G_io_apdu_buffer[i] = pubKey->W[64 - i];
    }
    if ((pubKey->W[KEY_SIZE] & 1) != 0) {
        G_io_apdu_buffer[KEY_SIZE-1] |= 0x80;
    }
}

// fetch public private key from 44' / 425'
void fetch_key_pair_from_bip32(cx_ecfp_public_key_t *publicKey, cx_ecfp_private_key_t *privateKey){
    uint32_t bip44_path[5];
    unsigned char *bip44_in = G_io_apdu_buffer + 2;
    uint32_t i;
    for (i = 0; i < 5; i++) {
        bip44_path[i] = (bip44_in[0] << 24) | (bip44_in[1] << 16) | (bip44_in[2] << 8) | (bip44_in[3]);
        bip44_in += 4;
    }

    unsigned char privateKeyData[KEY_SIZE];
    os_perso_derive_node_bip32(CX_CURVE_Ed25519, bip44_path, 5, privateKeyData, NULL);
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, KEY_SIZE, privateKey);
    cx_ecfp_init_public_key(CX_CURVE_Ed25519, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_Ed25519, publicKey, privateKey, 1);
}

// sign transaction
void sign_transaction(){
    // read transaction from buffer
    unsigned char transaction_hash[TRANSACTION_HASH_SIZE];
    os_memmove(transaction_hash, G_io_apdu_buffer+BIP_PATH_END, TRANSACTION_HASH_SIZE);

    // fetch public private key pair from bip32 path
    cx_ecfp_public_key_t publicKey;
    cx_ecfp_private_key_t privateKey;
    fetch_key_pair_from_bip32(&publicKey, &privateKey);

    // sign transaction
    unsigned int info = 0;
    unsigned int tx = 0;

    #if CX_APILEVEL >= 8
        tx = cx_eddsa_sign(
                &privateKey,
                CX_LAST,
                CX_SHA512,
                transaction_hash,
                TRANSACTION_HASH_SIZE,
                NULL,
                0,
                G_io_apdu_buffer,
                SIGNATURE_SIZE,
                &info);
    #else
        tx = cx_eddsa_sign(
                &privateKey,
                NULL,
                CX_LAST,
                CX_SHA512,
                transaction_hash,
                TRANSACTION_HASH_SIZE,
                G_io_apdu_buffer);
    #endif

    // clear keys
    MEMCLEAR(privateKey);
    MEMCLEAR(publicKey);
}

static const bagl_element_t* io_seproxyhal_touch_approve(const bagl_element_t *e) {
    unsigned int tx = 0;

    sign_transaction();
    tx = SIGNATURE_SIZE;

    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;

    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

static void aion_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // next timer callback in 500 ms
    UX_CALLBACK_SET_INTERVAL(500);

    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                if (G_io_apdu_buffer[0] != CLA) {
                    THROW(0x6E00);
                }

                switch (G_io_apdu_buffer[1]) {

                case INS_SIGN: {
                    // 2 byte for INS_GET_PUBLIC_KEY & 20 byte for bip32 & 32 byte for transaction hash
                    if (rx != BIP_PATH_END+TRANSACTION_HASH_SIZE) {
                        THROW(0x6D09);
                    }

                    // parse hex value and display on screen
                    G_io_apdu_buffer[BIP_PATH_END+TRANSACTION_HASH_SIZE] = '\0';
                    
                    #ifdef TESTING_ENABLED
                        sign_transaction();
                        tx = SIGNATURE_SIZE;
                        // return 0x9000 OK.
                        THROW(0x9000);
                        
                    #else

                        /*
			    We are directly sending transaction hash to Ledger Nano S as blue SDK doesn't support blake2b. 
			    Once user signs the hash, the signature is sent back. 

			    Note : Once the blue SDK start supporting blake2b, expect an update from us where we will be sending 
			    the RLP encoded transaction to Ledger Nano S for user to verify. Once user verifies it, the transaction 
			    hash will be generated on Nano S and signature will be sent back
				
                        */

                        populate_transaction();
                        show_transaction();
                        flags |= IO_ASYNCH_REPLY;

                    #endif
                } break;

                case INS_GET_PUBLIC_KEY: {
                    
                    // 2 byte for INS_GET_PUBLIC_KEY & 20 byte for bip32
                    if (rx != BIP_PATH_END) {
                        THROW(0x6D09);
                    }

                    G_io_apdu_buffer[BIP_PATH_END] = '\0';

                    // fetch public key from bip32
                    cx_ecfp_public_key_t publicKey;
                    cx_ecfp_private_key_t privateKey;
                    fetch_key_pair_from_bip32(&publicKey, &privateKey);

                    // extract key and write to output buffer
                    extract_public_key(&publicKey);
                    tx = KEY_SIZE;

                    // clear keys
                    MEMCLEAR(privateKey);
                    MEMCLEAR(publicKey);

                    // return 0x9000 OK.
                    THROW(0x9000);
                } break;

                case 0xFF: // return to dashboard
                    goto return_to_dashboard;

                default:
                    THROW(0x6D00);
                    break;
                }
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                case 0x9000:
                    sw = e;
                    break;
                default:
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

return_to_dashboard:
    return;
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    uiState = UI_IDLE;

    // ensure exception will work as planned
    os_boot();

    UX_INIT();

    BEGIN_TRY {
        TRY {
            io_seproxyhal_init();

            USB_power(0);
            USB_power(1);

            ui_idle();

            aion_main();
        }
        CATCH_OTHER(e) {
        }
        FINALLY {
        }
    }
    END_TRY;
}
