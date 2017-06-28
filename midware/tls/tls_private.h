/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2017, Alexey Kramarenko
    All rights reserved.
*/

#ifndef TLS_PRIVATE_H
#define TLS_PRIVATE_H

#include <stdint.h>
#include "../../userspace/tls.h"

#define TLS_NULL_WITH_NULL_NULL                                     0x0000
#define TLS_RSA_WITH_NULL_MD5                                       0x0001
#define TLS_RSA_WITH_NULL_SHA                                       0x0002
#define TLS_RSA_EXPORT_WITH_RC4_40_MD5                              0x0003
#define TLS_RSA_WITH_RC4_128_MD5                                    0x0004
#define TLS_RSA_WITH_RC4_128_SHA                                    0x0005
#define TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5                          0x0006
#define TLS_RSA_WITH_IDEA_CBC_SHA                                   0x0007
#define TLS_RSA_EXPORT_WITH_DES40_CBC_SHA                           0x0008
#define TLS_RSA_WITH_DES_CBC_SHA                                    0x0009
#define TLS_RSA_WITH_3DES_EDE_CBC_SHA                               0x000A
#define TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA                        0x000B
#define TLS_DH_DSS_WITH_DES_CBC_SHA                                 0x000C
#define TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA                            0x000D
#define TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA                        0x000E
#define TLS_DH_RSA_WITH_DES_CBC_SHA                                 0x000F
#define TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA                            0x0010
#define TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA                       0x0011
#define TLS_DHE_DSS_WITH_DES_CBC_SHA                                0x0012
#define TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA                           0x0013
#define TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA                       0x0014
#define TLS_DHE_RSA_WITH_DES_CBC_SHA                                0x0015
#define TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA                           0x0016
#define TLS_DH_anon_EXPORT_WITH_RC4_40_MD5                          0x0017
#define TLS_DH_anon_WITH_RC4_128_MD5                                0x0018
#define TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA                       0x0019
#define TLS_DH_anon_WITH_DES_CBC_SHA                                0x001A
#define TLS_DH_anon_WITH_3DES_EDE_CBC_SHA                           0x001B
#define TLS_KRB5_WITH_DES_CBC_SHA                                   0x001E
#define TLS_KRB5_WITH_3DES_EDE_CBC_SHA                              0x001F
#define TLS_KRB5_WITH_RC4_128_SHA                                   0x0020
#define TLS_KRB5_WITH_IDEA_CBC_SHA                                  0x0021
#define TLS_KRB5_WITH_DES_CBC_MD5                                   0x0022
#define TLS_KRB5_WITH_3DES_EDE_CBC_MD5                              0x0023
#define TLS_KRB5_WITH_RC4_128_MD5                                   0x0024
#define TLS_KRB5_WITH_IDEA_CBC_MD5                                  0x0025
#define TLS_KRB5_EXPORT_WITH_DES_CBC_40_SHA                         0x0026
#define TLS_KRB5_EXPORT_WITH_RC2_CBC_40_SHA                         0x0027
#define TLS_KRB5_EXPORT_WITH_RC4_40_SHA                             0x0028
#define TLS_KRB5_EXPORT_WITH_DES_CBC_40_MD5                         0x0029
#define TLS_KRB5_EXPORT_WITH_RC2_CBC_40_MD5                         0x002A
#define TLS_KRB5_EXPORT_WITH_RC4_40_MD5                             0x002B
#define TLS_PSK_WITH_NULL_SHA                                       0x002C
#define TLS_DHE_PSK_WITH_NULL_SHA                                   0x002D
#define TLS_RSA_PSK_WITH_NULL_SHA                                   0x002E
#define TLS_RSA_WITH_AES_128_CBC_SHA                                0x002F
#define TLS_DH_DSS_WITH_AES_128_CBC_SHA                             0x0030
#define TLS_DH_RSA_WITH_AES_128_CBC_SHA                             0x0031
#define TLS_DHE_DSS_WITH_AES_128_CBC_SHA                            0x0032
#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA                            0x0033
#define TLS_DH_anon_WITH_AES_128_CBC_SHA                            0x0034
#define TLS_RSA_WITH_AES_256_CBC_SHA                                0x0035
#define TLS_DH_DSS_WITH_AES_256_CBC_SHA                             0x0036
#define TLS_DH_RSA_WITH_AES_256_CBC_SHA                             0x0037
#define TLS_DHE_DSS_WITH_AES_256_CBC_SHA                            0x0038
#define TLS_DHE_RSA_WITH_AES_256_CBC_SHA                            0x0039
#define TLS_DH_anon_WITH_AES_256_CBC_SHA                            0x003A
#define TLS_RSA_WITH_NULL_SHA256                                    0x003B
#define TLS_RSA_WITH_AES_128_CBC_SHA256                             0x003C
#define TLS_RSA_WITH_AES_256_CBC_SHA256                             0x003D
#define TLS_DH_DSS_WITH_AES_128_CBC_SHA256                          0x003E
#define TLS_DH_RSA_WITH_AES_128_CBC_SHA256                          0x003F
#define TLS_DHE_DSS_WITH_AES_128_CBC_SHA256                         0x0040
#define TLS_RSA_WITH_CAMELLIA_128_CBC_SHA                           0x0041
#define TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA                        0x0042
#define TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA                        0x0043
#define TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA                       0x0044
#define TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA                       0x0045
#define TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA                       0x0046
#define TLS_DHE_RSA_WITH_AES_128_CBC_SHA256                         0x0067
#define TLS_DH_DSS_WITH_AES_256_CBC_SHA256                          0x0068
#define TLS_DH_RSA_WITH_AES_256_CBC_SHA256                          0x0069
#define TLS_DHE_DSS_WITH_AES_256_CBC_SHA256                         0x006A
#define TLS_DHE_RSA_WITH_AES_256_CBC_SHA256                         0x006B
#define TLS_DH_anon_WITH_AES_128_CBC_SHA256                         0x006C
#define TLS_DH_anon_WITH_AES_256_CBC_SHA256                         0x006D
#define TLS_RSA_WITH_CAMELLIA_256_CBC_SHA                           0x0084
#define TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA                        0x0085
#define TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA                        0x0086
#define TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA                       0x0087
#define TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA                       0x0088
#define TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA                       0x0089
#define TLS_PSK_WITH_RC4_128_SHA                                    0x008A
#define TLS_PSK_WITH_3DES_EDE_CBC_SHA                               0x008B
#define TLS_PSK_WITH_AES_128_CBC_SHA                                0x008C
#define TLS_PSK_WITH_AES_256_CBC_SHA                                0x008D
#define TLS_DHE_PSK_WITH_RC4_128_SHA                                0x008E
#define TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA                           0x008F
#define TLS_DHE_PSK_WITH_AES_128_CBC_SHA                            0x0090
#define TLS_DHE_PSK_WITH_AES_256_CBC_SHA                            0x0091
#define TLS_RSA_PSK_WITH_RC4_128_SHA                                0x0092
#define TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA                           0x0093
#define TLS_RSA_PSK_WITH_AES_128_CBC_SHA                            0x0094
#define TLS_RSA_PSK_WITH_AES_256_CBC_SHA                            0x0095
#define TLS_RSA_WITH_SEED_CBC_SHA                                   0x0096
#define TLS_DH_DSS_WITH_SEED_CBC_SHA                                0x0097
#define TLS_DH_RSA_WITH_SEED_CBC_SHA                                0x0098
#define TLS_DHE_DSS_WITH_SEED_CBC_SHA                               0x0099
#define TLS_DHE_RSA_WITH_SEED_CBC_SHA                               0x009A
#define TLS_DH_anon_WITH_SEED_CBC_SHA                               0x009B
#define TLS_RSA_WITH_AES_128_GCM_SHA256                             0x009C
#define TLS_RSA_WITH_AES_256_GCM_SHA384                             0x009D
#define TLS_DHE_RSA_WITH_AES_128_GCM_SHA256                         0x009E
#define TLS_DHE_RSA_WITH_AES_256_GCM_SHA384                         0x009F
#define TLS_DH_RSA_WITH_AES_128_GCM_SHA256                          0x00A0
#define TLS_DH_RSA_WITH_AES_256_GCM_SHA384                          0x00A1
#define TLS_DHE_DSS_WITH_AES_128_GCM_SHA256                         0x00A2
#define TLS_DHE_DSS_WITH_AES_256_GCM_SHA384                         0x00A3
#define TLS_DH_DSS_WITH_AES_128_GCM_SHA256                          0x00A4
#define TLS_DH_DSS_WITH_AES_256_GCM_SHA384                          0x00A5
#define TLS_DH_anon_WITH_AES_128_GCM_SHA256                         0x00A6
#define TLS_DH_anon_WITH_AES_256_GCM_SHA384                         0x00A7
#define TLS_PSK_WITH_AES_128_GCM_SHA256                             0x00A8
#define TLS_PSK_WITH_AES_256_GCM_SHA384                             0x00A9
#define TLS_DHE_PSK_WITH_AES_128_GCM_SHA256                         0x00AA
#define TLS_DHE_PSK_WITH_AES_256_GCM_SHA384                         0x00AB
#define TLS_RSA_PSK_WITH_AES_128_GCM_SHA256                         0x00AC
#define TLS_RSA_PSK_WITH_AES_256_GCM_SHA384                         0x00AD
#define TLS_PSK_WITH_AES_128_CBC_SHA256                             0x00AE
#define TLS_PSK_WITH_AES_256_CBC_SHA384                             0x00AF
#define TLS_PSK_WITH_NULL_SHA256                                    0x00B0
#define TLS_PSK_WITH_NULL_SHA384                                    0x00B1
#define TLS_DHE_PSK_WITH_AES_128_CBC_SHA256                         0x00B2
#define TLS_DHE_PSK_WITH_AES_256_CBC_SHA384                         0x00B3
#define TLS_DHE_PSK_WITH_NULL_SHA256                                0x00B4
#define TLS_DHE_PSK_WITH_NULL_SHA384                                0x00B5
#define TLS_RSA_PSK_WITH_AES_128_CBC_SHA256                         0x00B6
#define TLS_RSA_PSK_WITH_AES_256_CBC_SHA384                         0x00B7
#define TLS_RSA_PSK_WITH_NULL_SHA256                                0x00B8
#define TLS_RSA_PSK_WITH_NULL_SHA384                                0x00B9
#define TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256                        0x00BA
#define TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA256                     0x00BB
#define TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA256                     0x00BC
#define TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256                    0x00BD
#define TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256                    0x00BE
#define TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA256                    0x00BF
#define TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256                        0x00C0
#define TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA256                     0x00C1
#define TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA256                     0x00C2
#define TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA256                    0x00C3
#define TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256                    0x00C4
#define TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA256                    0x00C5
#define TLS_EMPTY_RENEGOTIATION_INFO_SCSV                           0x00FF
#define TLS_FALLBACK_SCSV                                           0x5600
#define TLS_ECDH_ECDSA_WITH_NULL_SHA                                0xC001
#define TLS_ECDH_ECDSA_WITH_RC4_128_SHA                             0xC002
#define TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA                        0xC003
#define TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA                         0xC004
#define TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA                         0xC005
#define TLS_ECDHE_ECDSA_WITH_NULL_SHA                               0xC006
#define TLS_ECDHE_ECDSA_WITH_RC4_128_SHA                            0xC007
#define TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA                       0xC008
#define TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA                        0xC009
#define TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA                        0xC00A
#define TLS_ECDH_RSA_WITH_NULL_SHA                                  0xC00B
#define TLS_ECDH_RSA_WITH_RC4_128_SHA                               0xC00C
#define TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA                          0xC00D
#define TLS_ECDH_RSA_WITH_AES_128_CBC_SHA                           0xC00E
#define TLS_ECDH_RSA_WITH_AES_256_CBC_SHA                           0xC00F
#define TLS_ECDHE_RSA_WITH_NULL_SHA                                 0xC010
#define TLS_ECDHE_RSA_WITH_RC4_128_SHA                              0xC011
#define TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA                         0xC012
#define TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA                          0xC013
#define TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA                          0xC014
#define TLS_ECDH_anon_WITH_NULL_SHA                                 0xC015
#define TLS_ECDH_anon_WITH_RC4_128_SHA                              0xC016
#define TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA                         0xC017
#define TLS_ECDH_anon_WITH_AES_128_CBC_SHA                          0xC018
#define TLS_ECDH_anon_WITH_AES_256_CBC_SHA                          0xC019
#define TLS_SRP_SHA_WITH_3DES_EDE_CBC_SHA                           0xC01A
#define TLS_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA                       0xC01B
#define TLS_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA                       0xC01C
#define TLS_SRP_SHA_WITH_AES_128_CBC_SHA                            0xC01D
#define TLS_SRP_SHA_RSA_WITH_AES_128_CBC_SHA                        0xC01E
#define TLS_SRP_SHA_DSS_WITH_AES_128_CBC_SHA                        0xC01F
#define TLS_SRP_SHA_WITH_AES_256_CBC_SHA                            0xC020
#define TLS_SRP_SHA_RSA_WITH_AES_256_CBC_SHA                        0xC021
#define TLS_SRP_SHA_DSS_WITH_AES_256_CBC_SHA                        0xC022
#define TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256                     0xC023
#define TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384                     0xC024
#define TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256                      0xC025
#define TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384                      0xC026
#define TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256                       0xC027
#define TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384                       0xC028
#define TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256                        0xC029
#define TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384                        0xC02A
#define TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256                     0xC02B
#define TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384                     0xC02C
#define TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256                      0xC02D
#define TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384                      0xC02E
#define TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256                       0xC02F
#define TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384                       0xC030
#define TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256                        0xC031
#define TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384                        0xC032
#define TLS_ECDHE_PSK_WITH_RC4_128_SHA                              0xC033
#define TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA                         0xC034
#define TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA                          0xC035
#define TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA                          0xC036
#define TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256                       0xC037
#define TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384                       0xC038
#define TLS_ECDHE_PSK_WITH_NULL_SHA                                 0xC039
#define TLS_ECDHE_PSK_WITH_NULL_SHA256                              0xC03A
#define TLS_ECDHE_PSK_WITH_NULL_SHA384                              0xC03B
#define TLS_RSA_WITH_ARIA_128_CBC_SHA256                            0xC03C
#define TLS_RSA_WITH_ARIA_256_CBC_SHA384                            0xC03D
#define TLS_DH_DSS_WITH_ARIA_128_CBC_SHA256                         0xC03E
#define TLS_DH_DSS_WITH_ARIA_256_CBC_SHA384                         0xC03F
#define TLS_DH_RSA_WITH_ARIA_128_CBC_SHA256                         0xC040
#define TLS_DH_RSA_WITH_ARIA_256_CBC_SHA384                         0xC041
#define TLS_DHE_DSS_WITH_ARIA_128_CBC_SHA256                        0xC042
#define TLS_DHE_DSS_WITH_ARIA_256_CBC_SHA384                        0xC043
#define TLS_DHE_RSA_WITH_ARIA_128_CBC_SHA256                        0xC044
#define TLS_DHE_RSA_WITH_ARIA_256_CBC_SHA384                        0xC045
#define TLS_DH_anon_WITH_ARIA_128_CBC_SHA256                        0xC046
#define TLS_DH_anon_WITH_ARIA_256_CBC_SHA384                        0xC047
#define TLS_ECDHE_ECDSA_WITH_ARIA_128_CBC_SHA256                    0xC048
#define TLS_ECDHE_ECDSA_WITH_ARIA_256_CBC_SHA384                    0xC049
#define TLS_ECDH_ECDSA_WITH_ARIA_128_CBC_SHA256                     0xC04A
#define TLS_ECDH_ECDSA_WITH_ARIA_256_CBC_SHA384                     0xC04B
#define TLS_ECDHE_RSA_WITH_ARIA_128_CBC_SHA256                      0xC04C
#define TLS_ECDHE_RSA_WITH_ARIA_256_CBC_SHA384                      0xC04D
#define TLS_ECDH_RSA_WITH_ARIA_128_CBC_SHA256                       0xC04E
#define TLS_ECDH_RSA_WITH_ARIA_256_CBC_SHA384                       0xC04F
#define TLS_RSA_WITH_ARIA_128_GCM_SHA256                            0xC050
#define TLS_RSA_WITH_ARIA_256_GCM_SHA384                            0xC051
#define TLS_DHE_RSA_WITH_ARIA_128_GCM_SHA256                        0xC052
#define TLS_DHE_RSA_WITH_ARIA_256_GCM_SHA384                        0xC053
#define TLS_DH_RSA_WITH_ARIA_128_GCM_SHA256                         0xC054
#define TLS_DH_RSA_WITH_ARIA_256_GCM_SHA384                         0xC055
#define TLS_DHE_DSS_WITH_ARIA_128_GCM_SHA256                        0xC056
#define TLS_DHE_DSS_WITH_ARIA_256_GCM_SHA384                        0xC057
#define TLS_DH_DSS_WITH_ARIA_128_GCM_SHA256                         0xC058
#define TLS_DH_DSS_WITH_ARIA_256_GCM_SHA384                         0xC059
#define TLS_DH_anon_WITH_ARIA_128_GCM_SHA256                        0xC05A
#define TLS_DH_anon_WITH_ARIA_256_GCM_SHA384                        0xC05B
#define TLS_ECDHE_ECDSA_WITH_ARIA_128_GCM_SHA256                    0xC05C
#define TLS_ECDHE_ECDSA_WITH_ARIA_256_GCM_SHA384                    0xC05D
#define TLS_ECDH_ECDSA_WITH_ARIA_128_GCM_SHA256                     0xC05E
#define TLS_ECDH_ECDSA_WITH_ARIA_256_GCM_SHA384                     0xC05F
#define TLS_ECDHE_RSA_WITH_ARIA_128_GCM_SHA256                      0xC060
#define TLS_ECDHE_RSA_WITH_ARIA_256_GCM_SHA384                      0xC061
#define TLS_ECDH_RSA_WITH_ARIA_128_GCM_SHA256                       0xC062
#define TLS_ECDH_RSA_WITH_ARIA_256_GCM_SHA384                       0xC063
#define TLS_PSK_WITH_ARIA_128_CBC_SHA256                            0xC064
#define TLS_PSK_WITH_ARIA_256_CBC_SHA384                            0xC065
#define TLS_DHE_PSK_WITH_ARIA_128_CBC_SHA256                        0xC066
#define TLS_DHE_PSK_WITH_ARIA_256_CBC_SHA384                        0xC067
#define TLS_RSA_PSK_WITH_ARIA_128_CBC_SHA256                        0xC068
#define TLS_RSA_PSK_WITH_ARIA_256_CBC_SHA384                        0xC069
#define TLS_PSK_WITH_ARIA_128_GCM_SHA256                            0xC06A
#define TLS_PSK_WITH_ARIA_256_GCM_SHA384                            0xC06B
#define TLS_DHE_PSK_WITH_ARIA_128_GCM_SHA256                        0xC06C
#define TLS_DHE_PSK_WITH_ARIA_256_GCM_SHA384                        0xC06D
#define TLS_RSA_PSK_WITH_ARIA_128_GCM_SHA256                        0xC06E
#define TLS_RSA_PSK_WITH_ARIA_256_GCM_SHA384                        0xC06F
#define TLS_ECDHE_PSK_WITH_ARIA_128_CBC_SHA256                      0xC070
#define TLS_ECDHE_PSK_WITH_ARIA_256_CBC_SHA384                      0xC071
#define TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256                0xC072
#define TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384                0xC073
#define TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256                 0xC074
#define TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384                 0xC075
#define TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256                  0xC076
#define TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384                  0xC077
#define TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256                   0xC078
#define TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384                   0xC079
#define TLS_RSA_WITH_CAMELLIA_128_GCM_SHA256                        0xC07A
#define TLS_RSA_WITH_CAMELLIA_256_GCM_SHA384                        0xC07B
#define TLS_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256                    0xC07C
#define TLS_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384                    0xC07D
#define TLS_DH_RSA_WITH_CAMELLIA_128_GCM_SHA256                     0xC07E
#define TLS_DH_RSA_WITH_CAMELLIA_256_GCM_SHA384                     0xC07F
#define TLS_DHE_DSS_WITH_CAMELLIA_128_GCM_SHA256                    0xC080
#define TLS_DHE_DSS_WITH_CAMELLIA_256_GCM_SHA384                    0xC081
#define TLS_DH_DSS_WITH_CAMELLIA_128_GCM_SHA256                     0xC082
#define TLS_DH_DSS_WITH_CAMELLIA_256_GCM_SHA384                     0xC083
#define TLS_DH_anon_WITH_CAMELLIA_128_GCM_SHA256                    0xC084
#define TLS_DH_anon_WITH_CAMELLIA_256_GCM_SHA384                    0xC085
#define TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256                0xC086
#define TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384                0xC087
#define TLS_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256                 0xC088
#define TLS_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384                 0xC089
#define TLS_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256                  0xC08A
#define TLS_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384                  0xC08B
#define TLS_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256                   0xC08C
#define TLS_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384                   0xC08D
#define TLS_PSK_WITH_CAMELLIA_128_GCM_SHA256                        0xC08E
#define TLS_PSK_WITH_CAMELLIA_256_GCM_SHA384                        0xC08F
#define TLS_DHE_PSK_WITH_CAMELLIA_128_GCM_SHA256                    0xC090
#define TLS_DHE_PSK_WITH_CAMELLIA_256_GCM_SHA384                    0xC091
#define TLS_RSA_PSK_WITH_CAMELLIA_128_GCM_SHA256                    0xC092
#define TLS_RSA_PSK_WITH_CAMELLIA_256_GCM_SHA384                    0xC093
#define TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256                        0xC094
#define TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384                        0xC095
#define TLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256                    0xC096
#define TLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384                    0xC097
#define TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256                    0xC098
#define TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384                    0xC099
#define TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256                  0xC09A
#define TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384                  0xC09B
#define TLS_RSA_WITH_AES_128_CCM                                    0xC09C
#define TLS_RSA_WITH_AES_256_CCM                                    0xC09D
#define TLS_DHE_RSA_WITH_AES_128_CCM                                0xC09E
#define TLS_DHE_RSA_WITH_AES_256_CCM                                0xC09F
#define TLS_RSA_WITH_AES_128_CCM_8                                  0xC0A0
#define TLS_RSA_WITH_AES_256_CCM_8                                  0xC0A1
#define TLS_DHE_RSA_WITH_AES_128_CCM_8                              0xC0A2
#define TLS_DHE_RSA_WITH_AES_256_CCM_8                              0xC0A3
#define TLS_PSK_WITH_AES_128_CCM                                    0xC0A4
#define TLS_PSK_WITH_AES_256_CCM                                    0xC0A5
#define TLS_DHE_PSK_WITH_AES_128_CCM                                0xC0A6
#define TLS_DHE_PSK_WITH_AES_256_CCM                                0xC0A7
#define TLS_PSK_WITH_AES_128_CCM_8                                  0xC0A8
#define TLS_PSK_WITH_AES_256_CCM_8                                  0xC0A9
#define TLS_PSK_DHE_WITH_AES_128_CCM_8                              0xC0AA
#define TLS_PSK_DHE_WITH_AES_256_CCM_8                              0xC0AB
#define TLS_ECDHE_ECDSA_WITH_AES_128_CCM                            0xC0AC
#define TLS_ECDHE_ECDSA_WITH_AES_256_CCM                            0xC0AD
#define TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8                          0xC0AE
#define TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8                          0xC0AF

#define TLS_COMPRESSION_NULL                                        0
#define TLS_COMPRESSION_DEFLATE                                     1
#define TLS_COMPRESSION_LZS                                         64

#define TLS_EXTENSION_SERVER_NAME                                   0
#define TLS_EXTENSION_MAX_FRAGMENT_LENGTH                           1
#define TLS_EXTENSION_CLIENT_CERTIFICATE_URL                        2
#define TLS_EXTENSION_TRUSTED_CA_KEYS                               3
#define TLS_EXTENSION_TRUSTED_HMAC                                  4
#define TLS_EXTENSION_STATUS_REQUEST                                5
#define TLS_EXTENSION_USER_MAPPING                                  6
#define TLS_EXTENSION_CLIENT_AUTHZ                                  7
#define TLS_EXTENSION_SERVER_AUTHZ                                  8
#define TLS_EXTENSION_CERT_TYPE                                     9
#define TLS_EXTENSION_SUPPORTED_GROUPS                              10
#define TLS_EXTENSION_EC_POINT_FORMATS                              11
#define TLS_EXTENSION_SRP                                           12
#define TLS_EXTENSION_SIGNATURE_ALGORITHMS                          13
#define TLS_EXTENSION_USE_SRTP                                      14
#define TLS_EXTENSION_HEARTBEAT                                     15
#define TLS_EXTENSION_APP_LAYER_PROTOCOL_NEGOTIATION                16
#define TLS_EXTENSION_STATUS_REQUEST_V2                             17
#define TLS_EXTENSION_SIGNED_CERTIFICATE_TIMESTAMP                  18
#define TLS_EXTENSION_CLIENT_CERTIFICATE_TYPE                       19
#define TLS_EXTENSION_SERVER_CERTIFICATE_TYPE                       20
#define TLS_EXTENSION_PADDING                                       21
#define TLS_EXTENSION_ENCRYPT_THEN_MAC                              22
#define TLS_EXTENSION_EXTENDED_MASTER_SECRET                        23
#define TLS_EXTENSION_SESSION_TICKET_TLS                            35
#define TLS_EXTENSION_RENEGOTIATION_INFO                            65281

typedef enum {
    TLS_KEY_EXCHANGE_NULL,
    TLS_KEY_EXCHANGE_RSA,
    TLS_KEY_EXCHANGE_DH_DSS,
    TLS_KEY_EXCHANGE_DH_RSA,
    TLS_KEY_EXCHANGE_DHE_DSS,
    TLS_KEY_EXCHANGE_DHE_RSA,
    TLS_KEY_EXCHANGE_DH_anon,
    TLS_KEY_EXCHANGE_KRB5,
    TLS_KEY_EXCHANGE_PSK,
    TLS_KEY_EXCHANGE_DHE_PSK,
    TLS_KEY_EXCHANGE_RSA_PSK,
    TLS_KEY_EXCHANGE_ECDH_ECDSA,
    TLS_KEY_EXCHANGE_ECDHE_ECDSA,
    TLS_KEY_EXCHANGE_ECDH_RSA,
    TLS_KEY_EXCHANGE_ECDHE_RSA,
    TLS_KEY_EXCHANGE_ECDH_anon,
    TLS_KEY_EXCHANGE_SRP_SHA,
    TLS_KEY_EXCHANGE_SRP_SHA_RSA,
    TLS_KEY_EXCHANGE_SRP_SHA_DSS,
    TLS_KEY_EXCHANGE_ECDHE_PSK,
    TLS_KEY_EXCHANGE_UNKNOWN
} TLS_KEY_EXCHANGE_TYPE;

typedef enum {
    TLS_CIPHER_NULL,
    TLS_CIPHER_RC4_40,
    TLS_CIPHER_RC4_128,
    TLS_CIPHER_RC2_CBC_40,
    TLS_CIPHER_IDEA_CBC,
    TLS_CIPHER_DES40_CBC,
    TLS_CIPHER_DES_CBC_40,
    TLS_CIPHER_DES_CBC,
    TLS_CIPHER_3DES_EDE_CBC,
    TLS_CIPHER_SEED_CBC,
    TLS_CIPHER_AES_128_CBC,
    TLS_CIPHER_AES_256_CBC,
    TLS_CIPHER_AES_128_GCM,
    TLS_CIPHER_AES_256_GCM,
    TLS_CIPHER_AES_128_CCM,
    TLS_CIPHER_AES_256_CCM,
    TLS_CIPHER_AES_128_CCM_8,
    TLS_CIPHER_AES_256_CCM_8,
    TLS_CIPHER_CAMELIA_128_CBC,
    TLS_CIPHER_CAMELIA_256_CBC,
    TLS_CIPHER_CAMELIA_128_GCM,
    TLS_CIPHER_CAMELIA_256_GCM,
    TLS_CIPHER_ARIA_128_CBC,
    TLS_CIPHER_ARIA_256_CBC,
    TLS_CIPHER_ARIA_128_GCM,
    TLS_CIPHER_ARIA_256_GCM,
    TLS_CIPHER_UNKNOWN
} TLS_CIPHER_TYPE;

typedef enum {
    TLS_HASH_NULL,
    TLS_HASH_MD5,
    TLS_HASH_SHA,
    TLS_HASH_SHA256,
    TLS_HASH_SHA384,
    TLS_HASH_NIL,
    TLS_HASH_UNKNOWN
} TLS_HASH_TYPE;

typedef enum {
    TLS_PROTOCOL_VERSION_UNSUPPORTED = 0,
    TLS_PROTOCOL_1_0,
    TLS_PROTOCOL_1_1,
    TLS_PROTOCOL_1_2
} TLS_PROTOCOL_VERSION;

typedef enum {
    TLS_CONTENT_CHANGE_CIPHER = 20,
    TLS_CONTENT_ALERT,
    TLS_CONTENT_HANDSHAKE,
    TLS_CONTENT_APP
} TLS_CONTENT_TYPE;

typedef enum {
    TLS_HANDSHAKE_HELLO_REQUEST = 0,
    TLS_HANDSHAKE_CLIENT_HELLO,
    TLS_HANDSHAKE_SERVER_HELLO,
    TLS_HANDSHAKE_CERTIFICATE = 11,
    TLS_HANDSHAKE_SERVER_KEY_EXCHANGE,
    TLS_HANDSHAKE_CERTIFICATE_REQUEST,
    TLS_HANDSHAKE_SERVER_HELLO_DONE,
    TLS_HANDSHAKE_CERTIFICATE_VERIFY,
    TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE,
    TLS_HANDSHAKE_FINISHED = 20
} TLS_HANDSHAKE_TYPE;

#define TLS_CHANGE_CIPHER_SPEC                                      1
#define TLS_FINISHED_DIGEST_SIZE                                    12

typedef enum {
    TLS_ALERT_LEVEL_WARNING = 1,
    TLS_ALERT_LEVEL_FATAL
} TLS_ALERT_LEVEL;

typedef enum {
    TLS_ALERT_CLOSE_NOTIFY,
    TLS_ALERT_UNEXPECTED_MESSAGE = 10,
    TLS_ALERT_BAD_RECORD_MAC = 20,
    TLS_ALERT_DECRYPTION_FAILED,
    TLS_ALERT_RECORD_OVERFLOW,
    TLS_ALERT_DECOMPRESSION_FAILURE = 30,
    TLS_ALERT_HANDSHAKE_FAILURE = 40,
    TLS_ALERT_NO_CERTIFICATE,
    TLS_ALERT_BAD_CERTIFICATE,
    TLS_ALERT_UNSUPPORTED_CERTIFICATE,
    TLS_ALERT_CERTIFICATE_REVOKED,
    TLS_ALERT_CERTIFICATE_EXPIRED,
    TLS_ALERT_CERTIFICATE_UNKNOWN,
    TLS_ALERT_ILLEGAL_PARAMETER,
    TLS_ALERT_UNKNOWN_CA,
    TLS_ALERT_ACCESS_DENIED,
    TLS_ALERT_DECODE_ERROR,
    TLS_ALERT_DECRYPT_ERROR,
    TLS_ALERT_EXPORT_RESTRICTION = 60,
    TLS_ALERT_PROTOCOL_VERSION = 70,
    TLS_ALERT_INSUFFICIENT_SECURITY,
    TLS_ALERT_INTERNAL_ERROR = 80,
    TLS_ALERT_USER_CANCELLED = 90,
    TLS_ALERT_NO_RENEGOTIATION = 100,
    TLS_ALERT_UNSUPPORTED_EXTENSION = 110
} TLS_ALERT_DESCRIPTION;

#pragma pack(push, 1)

typedef struct {
    uint8_t major;
    uint8_t minor;
} TLS_VERSION;

typedef struct {
    uint8_t content_type;
    TLS_VERSION version;
    uint8_t record_length_be[2];
} TLS_RECORD;

typedef struct {
    uint8_t size_hi;
    uint8_t size_lo_be[2];
} TLS_SIZE;

typedef struct {
    uint8_t message_type;
    TLS_SIZE message_length_be;
} TLS_HANDSHAKE;

typedef struct {
    uint8_t alert_level;
    uint8_t alert_description;
} TLS_ALERT;

typedef struct {
    TLS_VERSION version;
    uint8_t random[TLS_RANDOM_SIZE];
    uint8_t session_id_length;
} TLS_HELLO;

typedef struct {
    uint8_t code_be[2];
    uint8_t len_be[2];
} TLS_EXTENSION;

#pragma pack(pop)


#endif // TLS_PRIVATE_H
