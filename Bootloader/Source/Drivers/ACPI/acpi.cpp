#include <Drivers/ACPI/acpi.hpp>
#include <print.hpp>

using namespace Bootloader::ACPI;

constexpr GUID ACPI_GUID_2_0 = { 0x8868E871,0xE4F1,0x11D3,{0xBC,0x22,0x00,0x80,0xC7,0x3C,0x88,0x81} };

ACPI::ACPI(ACPIDesc* desc, EFI_CONFIGURATION_TABLE* confTables, size_t tableCount)
{
	if (!Initialize(desc, confTables, tableCount)) {
		printf("[ACPI] [INIT] [ERROR]: Failed to initialize ACPI\r\n");
		HaltSystem();
	}
}

bool ACPI::Initialize(ACPIDesc* desc, EFI_CONFIGURATION_TABLE* confTables, size_t tableCount)
{
	if (!desc || !confTables) return false;

	this->desc = desc;

	for (size_t i = 0; i < tableCount; i++)
	{
		if (CompareMem(&confTables[i].VendorGuid, &ACPI_GUID_2_0, sizeof(GUID)) == 0)
		{
			desc->rsdp = reinterpret_cast<const RSDPDescV2*>(confTables[i].VendorTable);
			break;
		}
	}

	if (!desc->rsdp) return false;

	if (desc->rsdp->v1.Revision >= 2 && desc->rsdp->XsdtAddress) desc->xsdt = reinterpret_cast<const XSDT*>(desc->rsdp->XsdtAddress);
	else return false;

	return true;
}

const ACPISDTHeader* ACPI::FindTable(const char* signature)
{
	if (!desc || !desc->xsdt || !signature) return nullptr;

	uint64_t entriesCount = (desc->xsdt->header.Length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);

	for (uint64_t i = 0; i < entriesCount; i++)
	{
		const ACPISDTHeader* header = reinterpret_cast<const ACPISDTHeader*>(desc->xsdt->Ptr[i]);
		if (CompareMem(header->Signature, signature, 4) == 0) return header;
	}

	return nullptr;
}

bool ACPI::GetMCFG(MCFG* mcfg)
{
	if (!desc || !mcfg) return false;
	printf("passed\r\n");

	HaltSystem();

	const ACPISDTHeader* header = FindTable("MCFG");
	if (!header) return false;

	size_t tableSize = header->Length;
	const uint8_t* src = reinterpret_cast<const uint8_t*>(header);
	uint8_t* dst = reinterpret_cast<uint8_t*>(mcfg);

	for (size_t i = 0; i < tableSize; i++) dst[i] = src[i];

	return true;
}