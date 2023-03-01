/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    eth.h
  * @brief   This file contains all the function prototypes for
  *          the eth.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ETH_H__
#define __ETH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern ETH_HandleTypeDef heth;

/* USER CODE BEGIN Private defines */
#ifndef ETH_TX_DESC_CNT
#define ETH_TX_DESC_CNT         4U
#endif /* ETH_TX_DESC_CNT */

#ifndef ETH_RX_DESC_CNT
#define ETH_RX_DESC_CNT         4U
#endif /* ETH_RX_DESC_CNT */

/** @defgroup ETH_Tx_Packet_Attributes ETH Tx Packet Attributes
  * @{
  */
#define ETH_TX_PACKETS_FEATURES_CSUM          0x00000001U
#define ETH_TX_PACKETS_FEATURES_SAIC          0x00000002U
#define ETH_TX_PACKETS_FEATURES_VLANTAG       0x00000004U
#define ETH_TX_PACKETS_FEATURES_INNERVLANTAG  0x00000008U
#define ETH_TX_PACKETS_FEATURES_TSO           0x00000010U
#define ETH_TX_PACKETS_FEATURES_CRCPAD        0x00000020U

/** @defgroup ETH_Tx_Packet_Checksum_Control ETH Tx Packet Checksum Control
  * @{
  */
#define ETH_CHECKSUM_DISABLE                         ETH_DMATXDESC_CIC_BYPASS
#define ETH_CHECKSUM_IPHDR_INSERT                    ETH_DMATXDESC_CIC_IPV4HEADER
#define ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT            ETH_DMATXDESC_CIC_TCPUDPICMP_SEGMENT
#define ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC  ETH_DMATXDESC_CIC_TCPUDPICMP_FULL

/** @defgroup ETH_Tx_Packet_CRC_Pad_Control ETH Tx Packet CRC Pad Control
  * @{
  */
#define ETH_CRC_PAD_DISABLE      (uint32_t)(ETH_DMATXDESC_DP | ETH_DMATXDESC_DC)
#define ETH_CRC_PAD_INSERT       0x00000000U
#define ETH_CRC_INSERT           ETH_DMATXDESC_DP

/**
  * @brief  ETH Buffers List structure definition
  */
typedef struct __ETH_BufferTypeDef
{
  uint8_t *buffer;                /*<! buffer address */

  uint32_t len;                   /*<! buffer length */

  struct __ETH_BufferTypeDef *next; /*<! Pointer to the next buffer in the list */
} ETH_BufferTypeDef;

/**
  * @brief  Transmit Packet Configuration structure definition
  */
typedef struct
{
  uint32_t Attributes;              /*!< Tx packet HW features capabilities.
                                         This parameter can be a combination of @ref ETH_Tx_Packet_Attributes*/

  uint32_t Length;                  /*!< Total packet length   */

  ETH_BufferTypeDef *TxBuffer;      /*!< Tx buffers pointers */

  uint32_t SrcAddrCtrl;             /*!< Specifies the source address insertion control.
                                         This parameter can be a value of @ref ETH_Tx_Packet_Source_Addr_Control */

  uint32_t CRCPadCtrl;             /*!< Specifies the CRC and Pad insertion and replacement control.
                                        This parameter can be a value of @ref ETH_Tx_Packet_CRC_Pad_Control  */

  uint32_t ChecksumCtrl;           /*!< Specifies the checksum insertion control.
                                        This parameter can be a value of @ref ETH_Tx_Packet_Checksum_Control  */

  uint32_t MaxSegmentSize;         /*!< Sets TCP maximum segment size only when TCP segmentation is enabled.
                                        This parameter can be a value from 0x0 to 0x3FFF */

  uint32_t PayloadLen;             /*!< Sets Total payload length only when TCP segmentation is enabled.
                                        This parameter can be a value from 0x0 to 0x3FFFF */

  uint32_t TCPHeaderLen;           /*!< Sets TCP header length only when TCP segmentation is enabled.
                                        This parameter can be a value from 0x5 to 0xF */

  uint32_t VlanTag;                /*!< Sets VLAN Tag only when VLAN is enabled.
                                        This parameter can be a value from 0x0 to 0xFFFF*/

  uint32_t VlanCtrl;               /*!< Specifies VLAN Tag insertion control only when VLAN is enabled.
                                        This parameter can be a value of @ref ETH_Tx_Packet_VLAN_Control */

  uint32_t InnerVlanTag;           /*!< Sets Inner VLAN Tag only when Inner VLAN is enabled.
                                        This parameter can be a value from 0x0 to 0x3FFFF */

  uint32_t InnerVlanCtrl;          /*!< Specifies Inner VLAN Tag insertion control only when Inner VLAN is enabled.
                                        This parameter can be a value of @ref ETH_Tx_Packet_Inner_VLAN_Control   */

  void *pData;                     /*!< Specifies Application packet pointer to save   */

} ETH_TxPacketConfig;

/**
  * @brief  HAL ETH Media Interfaces enum definition
  */
typedef enum
{
  HAL_ETH_MII_MODE             = 0x00U,   /*!<  Media Independent Interface               */
  HAL_ETH_RMII_MODE            = SYSCFG_PMC_MII_RMII_SEL    /*!<   Reduced Media Independent Interface       */
} ETH_MediaInterfaceTypeDef;



/* USER CODE END Private defines */

void MX_ETH_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ETH_H__ */

