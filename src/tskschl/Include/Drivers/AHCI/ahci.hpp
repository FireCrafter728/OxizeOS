#pragma once

#include <stdint.hpp>

#include <Drivers/PCIe/PCIe.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

namespace TskSchl
{
    namespace AHCI
    {
        constexpr uint8_t FIS_TYPE_H2D = 0x27;
        constexpr uint8_t FIS_TYPE_D2H = 0x34;
        constexpr uint8_t FIS_TYPE_DMA_SETUP = 0x39;
        constexpr uint8_t FIS_TYPE_PIO_SETUP = 0x5F;
        constexpr uint8_t FIS_TYPE_DEV_BITS = 0xA1;

        constexpr uint8_t FIS_COMMAND_IDENTIFY = 0xEC;
        constexpr uint8_t FIS_COMMAND_READ_DMA_EXT = 0x25;

        struct PACK Port
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

        struct PACK HBA
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
            Port Ports[32];
        };

        struct PACK HBACommandHeader
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

        struct PACK PRDTEntry
        {
            uint32_t DataBaseAddress, DataBaseAddressUpper;
            uint32_t _Reserved;
            uint32_t ByteCount;
        };

        struct PACK CommandTable
        {
            uint8_t CFIS[64];
            uint8_t ACMD[16];
            uint8_t _Reserved[48];
            PRDTEntry PRDT[];
        };

        struct PACK H2DFIS
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

        struct PACK D2HFIS
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

        struct PACK FISDMASetup
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

        struct PACK FISPIOSetup
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

        struct PACK FISDeviceBits
        {
            uint8_t FISType;

            uint8_t PMPort : 4;
            uint8_t _Reserved : 2;
            uint8_t interrupt : 1;
            uint8_t _Reserved1 : 1;

            uint8_t status, active;
        };

        enum HBACapabilities : uint32_t
        {
            HBA_CAP_PortCount = (0x1FU << 0),
            HBA_CAP_ExternalSATA = (1U << 5),
            HBA_CAP_EnclosureManagement = (1U << 6),
            HBA_CAP_CommandCompletionCoalescing = (1U << 7),
            HBA_CAP_CommandSlotCount = (0x1FU << 8),
            HBA_CAP_PartialStateCapable = (1U << 13),
            HBA_CAP_SlumberStateCapable = (1U << 14),
            HBA_CAP_PIOMultipleDRQBlock = (1U << 15),
            HBA_CAP_FISBasedSwitching = (1U << 16),
            HBA_CAP_SupportsPortMultiplier = (1U << 17),
            HBA_CAP_AHCIOnly = (1U << 18),
            HBA_CAP_StaggeredSpinUp = (1U << 19),
            HBA_CAP_AggressiveLinkPowerManagement = (1U << 20),
            HBA_CAP_ActivityLED = (1U << 21),
            HBA_CAP_CommandListOverride = (1U << 22),
            HBA_CAP_NativeCommandQueuing = (1U << 23),
            HBA_CAP_SNotificationReg = (1U << 24),
            HBA_CAP_MechanicalPresenceSwitch = (1U << 25),
            HBA_CAP_SCSIEnclosureManagement = (1U << 31),
        };

        enum GHC : uint32_t
        {
            GHC_AHCIEnable = (1U << 31),
            GHC_MSIRevertToSingleMessage = (1U << 2),
            GHC_InterruptEnable = (1U << 1),
            GHC_HBAReset = 1U,
        };

        enum HBACapabilitiesEx : uint32_t
        {
            HBA_CAP_EX_DevSleepEntranceFromSlumberOnly = (1U << 5),
            HBA_CAP_EX_AggressiveDeviceSleepManagement = (1U << 4),
            HBA_CAP_EX_DeviceSleep = (1U << 3),
            HBA_CAP_EX_AutoPartialToSlumberTransitions = (1U << 2),
            HBA_CAP_EX_NVMHCIPresent = (1U << 1),
            HBA_CAP_EX_BIOSHandoff = 1U,
        };

        enum PxIS : uint32_t
        {
            PxIS_DeviceToHostRegisterFISInterrupt = 1U,
            PxIS_PIOSetupFISInterrupt = (1U << 1),
            PxIS_DMASetupFISInterrupt = (1U << 2),
            PxIS_SetDeviceBitsInterrupt = (1U << 3),
            PxIS_UnknownFISInterrupt = (1U << 4),
            PxIS_DescriptorProcessed = (1U << 5),
            PxIS_PortConnectChangeStatus = (1U << 6),
            PxIS_DeviceMechanicalPresenceStatus = (1U << 7),
            PxIS_PhyRdyChangeStatus = (1U << 22),
            PxIS_IncorrectPortMultiplierStatus = (1U << 23),
            PxIS_OverflowStatus = (1U << 24),
            PxIS_NonFatalErrorStatus = (1U << 26),
            PxIS_FatalErrorStatus = (1U << 27),
            PxIS_HostBusDataErrorStatus = (1U << 28),
            PxIS_HostBusFatalErrorStatus = (1U << 29),
            PxIS_TaskFileErrorStatus = (1U << 30),
            PxIS_ColdPortDetectStatus = (1U << 31),
        };

        enum PxIE : uint32_t
        {
            PxIE_DeviceToHostRegisterInterruptEnable = 1U,
            PxIE_PIOSetupFISInterruptEnable = (1U << 1),
            PxIE_DMASetupFISInterruptEnable = (1U << 2),
            PxIE_SetDeviceBitsFISInterruptEnable = (1U << 3),
            PxIE_UnknownFISInterruptEnable = (1U << 4),
            PxIE_DescriptorProcessedInterruptEnable = (1U << 5),
            PxIE_PortChangeInterruptEnable = (1U << 6),
            PxIE_DeviceMechanicalPresenceEnable = (1U << 7),
            PxIE_PhyRdyInterruptEnable = (1U << 22),
            PxIE_IncorrectPortMultiplierEnable = (1U << 23),
            PxIE_OverflowEnable = (1U << 24),
            PxIE_NonFatalErrorEnable = (1U << 26),
            PxIE_FatalErrorEnable = (1U << 27),
            PxIE_HostBusDataErrorEnable = (1U << 28),
            PxIE_HostBusFatalErrorEnable = (1U << 29),
            PxIE_TaskFileErrorEnable = (1U << 30),
            PxIE_ColdPortDetectEnable = (1U << 31),
        };

        enum PxCMD : uint32_t
        {
            PxCMD_InterfaceCommunicationControl = (0x1FU << 28),
            PxCMD_AggressiveSlumberPartial = (1U << 27),
            PxCMD_AggressiveLinkPowerManagementEnable = (1U << 26),
            PxCMD_DriveLEDOnATAPIEnable = (1U << 25),
            PxCMD_DeviceIsATAPI = (1U << 24),
            PxCMD_AutoPartialToSlumberTransitionsEnabled = (1U << 23),
            PxCMD_FISBasedSwitchingCapablePort = (1U << 22),
            PxCMD_ExternalSATAPort = (1U << 21),
            PxCMD_ColdPresenceDetection = (1U << 20),
            PxCMD_MechanicalPresenceSwitchAttachedToPort = (1U << 19),
            PxCMD_HotPlugCapablePort = (1U << 18),
            PxCMD_PortMultiplierAttached = (1U << 17),
            PxCMD_ColdPresenceState = (1U << 16),
            PxCMD_CommandListRunning = (1U << 15),
            PxCMD_FISReceiveRunning = (1U << 14),
            PxCMD_MechanicalPresenceSwitchState = (1U << 13),
            PxCMD_CurrentCommandSlot = (0x1FU << 12),
            PxCMD_FISReceiveEnable = (1U << 4),
            PxCMD_CommandListOverride = (1U << 3),
            PxCMD_PowerOnDevice = (1U << 2),
            PxCMD_SpinUpDevice = (1U << 1),
            PxCMD_Start = 1U,
        };

        enum PxSERR : uint32_t
        {
            PxSERR_Exchanged = (1U << 26),
            PxSERR_UnknownFISType = (1U << 25),
            PxSERR_TransportSizeTransitionError = (1U << 24),
            PxSERR_LinkSequenceError = (1U << 23),
            PxSERR_HandshakeError = (1U << 22),
            PxSERR_CRCError = (1U << 21),
            PxSERR_10B8BDecodeError = (1U << 19),
            PxSERR_CommWake = (1U << 18),
            PxSERR_PhyInternalError = (1U << 17),
            PxSERR_PhyRdyChange = (1U << 16),
            PxSERR_InternalError = (1U << 11),
            PxSERR_ProtocolError = (1U << 10),
            PxSERR_PersistentCommunicationDataIntegrityError = (1U << 9),
            PxSERR_TranientDataIntegrityError = (1U << 8),
            PxSERR_RecoveredCommunicationsError = (1U << 1),
            PxSERR_RecoveredDataIntegrityError = 1U,
        };

        enum PxFBS : uint32_t
        {
            PxFBS_DeviceWithError = (0x1FU << 16),
            PxFBS_ActiveDeviceOptimization = (0x1FU << 12),
            PxFBS_DeviceToIssue = (0x1FU << 8),
            PxFBS_SingleDeviceError = (1U << 2),
            PxFBS_DeviceErrorClear = (1U << 1),
            PxFBS_Enable = 1U,
        };

        enum PxDEVSLP : uint32_t
        {
            PxDEVSLP_DITOMultiplier = (0x1FU << 25),
            PxDEVSLP_DeviceSleepIdleTimeout = (0x3FFU << 15),
            PxDEVSLP_MinimumDeviceSleepAssertionTime = (0x3FU << 10),
            PxDEVSLP_DeviceSleepExitTimeout = (0xFFU << 2),
            PxDEVSLP_DeviceSleepPresent = (1U << 1),
            PxDEVSLP_AggressiveDeviceSleepEnable = 1U,
        };

        enum PxTFD : uint32_t
        {
            PxTFD_ErrorReg = (0xFFU << 8),
            PxTFD_Busy = (1U << 7),
            PxTFD_CS0 = (1U << 6),
            PxTFD_CS1 = (1U << 5),
            PxTFD_CS2 = (1U << 4),
            PxTFD_DRQ = (1U << 3),
            PxTFD_CS3 = (1U << 2),
            PxTFD_CS4 = (1U << 1),
            PxTFD_Error = (1U << 0),
        };

        enum PortType
        {
            PxSIG_None = 0x00000000,
            PxSIG_SATA = 0x00000101,
            PxSIG_ATAPI = 0xEB140101,
            PxSIG_EnclosureMgmt = 0xC33C0101,
            PxSIG_PortMultiplier = 0x96690101,
        };

        struct PortDesc
        {
            volatile Port* port;
            volatile HBACommandHeader* CLB;
            volatile uint8_t* FISReceiveBuffer;
            volatile size_t FISReceiveBufferSize;
            volatile CommandTable* commandTables[32]; // Command table size is 4K
            volatile PortType type;
            uint8_t IdentifyBuffer[512];
        };

        struct AHCIDevice
        {
            PCIe::DeviceInfo* deviceInfo;
            volatile HBA* hba;
            PortDesc ports[32];
        };

        struct AHCIDiskDevice
        {
            AHCIDevice* controller;
            uint8_t devicePort;
        };

        class AHCI
        {
        public:
            AHCI() = default;
            AHCI(AHCIDevice* device);
            bool Initialize(AHCIDevice* device);
            bool ReadSectors(AHCIDiskDevice* diskDevice, uint64_t lba, size_t count, void* bufferOut);
        private:
            bool ResetPorts(AHCIDevice* device);
            bool InitializePorts(AHCIDevice* device);
        };
    }
}