// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <arch/x86_64/PCI/PCIe.hpp>

namespace krnl
{
	constexpr uint8_t AHCI_FIS_TYPE_H2D = 0x27;
	constexpr uint8_t AHCI_FIS_TYPE_D2H = 0x34;
	constexpr uint8_t AHCI_FIS_TYPE_DMA_SETUP = 0x39;
	constexpr uint8_t AHCI_FIS_TYPE_PIO_SETUP = 0x5F;
	constexpr uint8_t AHCI_FIS_TYPE_DEV_BITS = 0xA1;

	constexpr uint8_t AHCI_FIS_COMMAND_IDENTIFY = 0xEC;
	constexpr uint8_t AHCI_FIS_COMMAND_READ_DMA_EXT = 0x25;

	struct PACK AHCI_Port
	{
		uint32_t PxCLB;
		uint32_t PxCLBU;
		uint32_t PxFB;
		uint32_t PxFBU;
		uint32_t PxIS;
		uint32_t PxIE;
		uint32_t PxCMD;
		uint32_t _Reserved;
		uint32_t PxTFD;
		uint32_t PxSIG;
		uint32_t PxSSTS;
		uint32_t PxSCTL;
		uint32_t PxSERR;
		uint32_t PxSACT;
		uint32_t PxCI;
		uint32_t PxSNTF;
		uint32_t PxFBS;
		uint32_t PxDEVSLP;
		uint8_t _Reserved2[56];
	};

	struct PACK AHCI_HBA
	{
		uint32_t HostCapabilities;
		uint32_t GlobalControl;
		uint32_t InterruptStatus;
		uint32_t PortsImplemented;
		uint32_t Version;
		uint32_t CommandCompletionCoalescingControl;
		uint32_t CommandCompletionCoalescingPorts;
		uint32_t EnclosureManagementLocation;
		uint32_t EnclosureManagementControl;
		uint32_t HostCapabilitiesEx;
		uint32_t BIOSHandoffControlStatus;
		uint8_t _Reserved[0xA0 - 0x2C];
		uint8_t Vendor[0x100 - 0xA0];
		AHCI_Port Ports[32];
	};

	struct PACK AHCI_HBACommandHeader
	{
		uint8_t CommandFISLength : 5;
		uint8_t ATAPICommand : 1;
		uint8_t WriteDirection : 1;
		uint8_t Prefetchable : 1;
		uint8_t Reset : 1;
		uint8_t BIST : 1;
		uint8_t ClearBusy : 1;
		uint8_t _Reserved : 1;
		uint8_t PortMultiplier : 4;
		uint16_t PRDTLength;
		volatile uint32_t PRDBC;
		uint32_t CommandTableBase;
		uint32_t CommandTableBaseUpper;
		uint32_t _Reserved1[4];
	};

	struct PACK AHCI_PRDTEntry
	{
		uint32_t DataBaseAddress, DataBaseAddressUpper;
		uint32_t _Reserved;
		uint32_t IOC_ByteCount;
	};

	struct PACK AHCI_CommandTable
	{
		uint8_t CFIS[64];
		uint8_t ACMD[16];
		uint8_t _Reserved[48];
		AHCI_PRDTEntry PRDT[];
	};

	struct PACK AHCI_H2DFIS
	{
		uint8_t FISType;

		uint8_t PMPort : 4;
		uint8_t _Reserved : 3;
		uint8_t c : 1;
		uint8_t command, featurelevel;

		uint8_t lba0, lba1, lba2;

		uint8_t device;

		uint8_t lba3, lba4, lba5;

		uint8_t featureh;
		uint16_t count;

		uint8_t icc, control;
	};

	struct PACK AHCI_D2HFIS
	{
		uint8_t FISType;

		uint8_t PMPort : 4;
		uint8_t _Reserved : 2;
		uint8_t Interrupt : 1;
		uint8_t _Reserved1 : 1;

		uint8_t status, error;

		uint8_t lba0, lba1, lba2;

		uint8_t device;

		uint8_t lba3, lba4, lba5;

		uint8_t _Reserved2;
		uint16_t count;
	};

	struct PACK AHCI_FISDMASetup
	{
		uint8_t FISType;

		uint8_t PMPort : 4;
		uint8_t _Reserved : 1;
		uint8_t d : 1;
		uint8_t i : 1;
		uint8_t a : 1;

		uint16_t _Reserved1;

		uint64_t DMABufferID;

		uint32_t _Reserved2;

		uint32_t DMABufferOffset, TransferCount;
	};

	struct PACK AHCI_FISPIOSetup
	{
		uint8_t FISType;

		uint8_t PMPort : 4;
		uint8_t _Reserved : 1;
		uint8_t d : 1;
		uint8_t i : 1;
		uint8_t _Reserved1 : 1;

		uint8_t status, error;

		uint8_t lba0, lba1, lba2;

		uint8_t device;

		uint8_t lba3, lba4, lba5;

		uint8_t _Reserved2;
		uint16_t count;

		uint8_t _Reserved3;

		uint8_t eStatus;

		uint16_t TransferCount;
	};

	struct PACK AHCI_FISDeviceBits
	{
		uint8_t FISType;

		uint8_t PMPort : 4;
		uint8_t _Reserved : 2;
		uint8_t interrupt : 1;
		uint8_t _Reserved1 : 1;

		uint8_t status, active;
	};

	enum AHCI_HBACapabilities : uint32_t
	{
		AHCI_HBA_CAP_PortCount = (0x1FU << 0),
		AHCI_HBA_CAP_ExternalSATA = (1U << 5),
		AHCI_HBA_CAP_EnclosureManagement = (1U << 6),
		AHCI_HBA_CAP_CommandCompletionCoalescing = (1U << 7),
		AHCI_HBA_CAP_CommandSlotCount = (0x1FU << 8),
		AHCI_HBA_CAP_PartialStateCapable = (1U << 13),
		AHCI_HBA_CAP_SlumberStateCapable = (1U << 14),
		AHCI_HBA_CAP_PIOMultipleDRQBlock = (1U << 15),
		AHCI_HBA_CAP_FISBasedSwitching = (1U << 16),
		AHCI_HBA_CAP_SupportsPortMultiplier = (1U << 17),
		AHCI_HBA_CAP_AHCIOnly = (1U << 18),
		AHCI_HBA_CAP_StaggeredSpinUp = (1U << 19),
		AHCI_HBA_CAP_AggressiveLinkPowerManagement = (1U << 20),
		AHCI_HBA_CAP_ActivityLED = (1U << 21),
		AHCI_HBA_CAP_CommandListOverride = (1U << 22),
		AHCI_HBA_CAP_NativeCommandQueuing = (1U << 23),
		AHCI_HBA_CAP_SNotificationReg = (1U << 24),
		AHCI_HBA_CAP_MechanicalPresenceSwitch = (1U << 25),
		AHCI_HBA_CAP_SCSIEnclosureManagement = (1U << 31),
	};

	enum AHCI_GHC : uint32_t
	{
		AHCI_GHC_AHCIEnable = (1U << 31),
		AHCI_GHC_MSIRevertToSingleMessage = (1U << 2),
		AHCI_GHC_InterruptEnable = (1U << 1),
		AHCI_GHC_HBAReset = 1U,
	};

	enum AHCI_HBACapabilitiesEx : uint32_t
	{
		AHCI_HBA_CAP_EX_DevSleepEntranceFromSlumberOnly = (1U << 5),
		AHCI_HBA_CAP_EX_AggressiveDeviceSleepManagement = (1U << 4),
		AHCI_HBA_CAP_EX_DeviceSleep = (1U << 3),
		AHCI_HBA_CAP_EX_AutoPartialToSlumberTransitions = (1U << 2),
		AHCI_HBA_CAP_EX_NVMHCIPresent = (1U << 1),
		AHCI_HBA_CAP_EX_BIOSHandoff = 1U,
	};

	enum AHCI_PxIS : uint32_t
	{
		AHCI_PxIS_DeviceToHostRegisterFISInterrupt = 1U,
		AHCI_PxIS_PIOSetupFISInterrupt = (1U << 1),
		AHCI_PxIS_DMASetupFISInterrupt = (1U << 2),
		AHCI_PxIS_SetDeviceBitsInterrupt = (1U << 3),
		AHCI_PxIS_UnknownFISInterrupt = (1U << 4),
		AHCI_PxIS_DescriptorProcessed = (1U << 5),
		AHCI_PxIS_PortConnectChangeStatus = (1U << 6),
		AHCI_PxIS_DeviceMechanicalPresenceStatus = (1U << 7),
		AHCI_PxIS_PhyRdyChangeStatus = (1U << 22),
		AHCI_PxIS_IncorrectPortMultiplierStatus = (1U << 23),
		AHCI_PxIS_OverflowStatus = (1U << 24),
		AHCI_PxIS_NonFatalErrorStatus = (1U << 26),
		AHCI_PxIS_FatalErrorStatus = (1U << 27),
		AHCI_PxIS_HostBusDataErrorStatus = (1U << 28),
		AHCI_PxIS_HostBusFatalErrorStatus = (1U << 29),
		AHCI_PxIS_TaskFileErrorStatus = (1U << 30),
		AHCI_PxIS_ColdPortDetectStatus = (1U << 31),
	};

	enum AHCI_PxIE : uint32_t
	{
		AHCI_PxIE_DeviceToHostRegisterInterruptEnable = 1U,
		AHCI_PxIE_PIOSetupFISInterruptEnable = (1U << 1),
		AHCI_PxIE_DMASetupFISInterruptEnable = (1U << 2),
		AHCI_PxIE_SetDeviceBitsFISInterruptEnable = (1U << 3),
		AHCI_PxIE_UnknownFISInterruptEnable = (1U << 4),
		AHCI_PxIE_DescriptorProcessedInterruptEnable = (1U << 5),
		AHCI_PxIE_PortChangeInterruptEnable = (1U << 6),
		AHCI_PxIE_DeviceMechanicalPresenceEnable = (1U << 7),
		AHCI_PxIE_PhyRdyInterruptEnable = (1U << 22),
		AHCI_PxIE_IncorrectPortMultiplierEnable = (1U << 23),
		AHCI_PxIE_OverflowEnable = (1U << 24),
		AHCI_PxIE_NonFatalErrorEnable = (1U << 26),
		AHCI_PxIE_FatalErrorEnable = (1U << 27),
		AHCI_PxIE_HostBusDataErrorEnable = (1U << 28),
		AHCI_PxIE_HostBusFatalErrorEnable = (1U << 29),
		AHCI_PxIE_TaskFileErrorEnable = (1U << 30),
		AHCI_PxIE_ColdPortDetectEnable = (1U << 31),
	};

	enum AHCI_PxCMD : uint32_t
	{
		AHCI_PxCMD_InterfaceCommunicationControl = (0x1FU << 28),
		AHCI_PxCMD_AggressiveSlumberPartial = (1U << 27),
		AHCI_PxCMD_AggressiveLinkPowerManagementEnable = (1U << 26),
		AHCI_PxCMD_DriveLEDOnATAPIEnable = (1U << 25),
		AHCI_PxCMD_DeviceIsATAPI = (1U << 24),
		AHCI_PxCMD_AutoPartialToSlumberTransitionsEnabled = (1U << 23),
		AHCI_PxCMD_FISBasedSwitchingCapablePort = (1U << 22),
		AHCI_PxCMD_ExternalSATAPort = (1U << 21),
		AHCI_PxCMD_ColdPresenceDetection = (1U << 20),
		AHCI_PxCMD_MechanicalPresenceSwitchAttachedToPort = (1U << 19),
		AHCI_PxCMD_HotPlugCapablePort = (1U << 18),
		AHCI_PxCMD_PortMultiplierAttached = (1U << 17),
		AHCI_PxCMD_ColdPresenceState = (1U << 16),
		AHCI_PxCMD_CommandListRunning = (1U << 15),
		AHCI_PxCMD_FISReceiveRunning = (1U << 14),
		AHCI_PxCMD_MechanicalPresenceSwitchState = (1U << 13),
		AHCI_PxCMD_CurrentCommandSlot = (0x1FU << 12),
		AHCI_PxCMD_FISReceiveEnable = (1U << 4),
		AHCI_PxCMD_CommandListOverride = (1U << 3),
		AHCI_PxCMD_PowerOnDevice = (1U << 2),
		AHCI_PxCMD_SpinUpDevice = (1U << 1),
		AHCI_PxCMD_Start = 1U,
	};

	enum AHCI_PxSERR : uint32_t
	{
		AHCI_PxSERR_Exchanged = (1U << 26),
		AHCI_PxSERR_UnknownFISType = (1U << 25),
		AHCI_PxSERR_TransportSizeTransitionError = (1U << 24),
		AHCI_PxSERR_LinkSequenceError = (1U << 23),
		AHCI_PxSERR_HandshakeError = (1U << 22),
		AHCI_PxSERR_CRCError = (1U << 21),
		AHCI_PxSERR_10B8BDecodeError = (1U << 19),
		AHCI_PxSERR_CommWake = (1U << 18),
		AHCI_PxSERR_PhyInternalError = (1U << 17),
		AHCI_PxSERR_PhyRdyChange = (1U << 16),
		AHCI_PxSERR_InternalError = (1U << 11),
		AHCI_PxSERR_ProtocolError = (1U << 10),
		AHCI_PxSERR_PersistentCommunicationDataIntegrityError = (1U << 9),
		AHCI_PxSERR_TranientDataIntegrityError = (1U << 8),
		AHCI_PxSERR_RecoveredCommunicationsError = (1U << 1),
		AHCI_PxSERR_RecoveredDataIntegrityError = 1U,
	};

	enum AHCI_PxFBS : uint32_t
	{
		AHCI_PxFBS_DeviceWithError = (0x1FU << 16),
		AHCI_PxFBS_ActiveDeviceOptimization = (0x1FU << 12),
		AHCI_PxFBS_DeviceToIssue = (0x1FU << 8),
		AHCI_PxFBS_SingleDeviceError = (1U << 2),
		AHCI_PxFBS_DeviceErrorClear = (1U << 1),
		AHCI_PxFBS_Enable = 1U,
	};

	enum AHCI_PxDEVSLP : uint32_t
	{
		AHCI_PxDEVSLP_DITOMultiplier = (0x1FU << 25),
		AHCI_PxDEVSLP_DeviceSleepIdleTimeout = (0x3FFU << 15),
		AHCI_PxDEVSLP_MinimumDeviceSleepAssertionTime = (0x3FU << 10),
		AHCI_PxDEVSLP_DeviceSleepExitTimeout = (0xFFU << 2),
		AHCI_PxDEVSLP_DeviceSleepPresent = (1U << 1),
		AHCI_PxDEVSLP_AggressiveDeviceSleepEnable = 1U,
	};

	enum AHCI_PxTFD : uint32_t
	{
		AHCI_PxTFD_ErrorReg = (0xFFU << 8),
		AHCI_PxTFD_Busy = (1U << 7),
		AHCI_PxTFD_CS0 = (1U << 6),
		AHCI_PxTFD_CS1 = (1U << 5),
		AHCI_PxTFD_CS2 = (1U << 4),
		AHCI_PxTFD_DRQ = (1U << 3),
		AHCI_PxTFD_CS3 = (1U << 2),
		AHCI_PxTFD_CS4 = (1U << 1),
		AHCI_PxTFD_Error = (1U << 0),
	};

	enum AHCI_PortType
	{
		AHCI_PxSIG_None = 0x00000000,
		AHCI_PxSIG_SATA = 0x00000101,
		AHCI_PxSIG_ATAPI = 0xEB140101,
		AHCI_PxSIG_EnclosureMgmt = 0xC33C0101,
		AHCI_PxSIG_PortMultiplier = 0x96690101,
	};

	struct AHCI_PortDesc
	{
		volatile AHCI_Port* port;
		volatile AHCI_HBACommandHeader* CLB;
		volatile uint8_t* FISReceiveBuffer;
		volatile size_t FISReceiveBufferSize;
		AHCI_CommandTable* commandTables[32]; // Command table size is 4K
		volatile AHCI_PortType type;
		uint8_t IdentifyBuffer[512];
	};

	struct AHCIDevice
	{
		PCIe_DeviceInfo* deviceInfo;
		volatile AHCI_HBA* hba;
		AHCI_PortDesc ports[32];
	};

	struct AHCIDiskDevice
	{
		AHCIDevice* controller;
		uint8_t devicePort;
	};

	class AHCI
	{
	public:
		bool Initialize(AHCIDevice* device);
		bool ReadSectors(AHCIDiskDevice* diskDevice, uint64_t lba, size_t count, void* bufferOut);
	private:
		bool ResetPorts(AHCIDevice* device);
		bool InitializePorts(AHCIDevice* device);
	};
}