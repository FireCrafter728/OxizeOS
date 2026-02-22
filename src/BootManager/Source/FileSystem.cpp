#include <FileSystem.hpp>

using namespace BootMgr::FS;

EFI_HANDLE FS::GetVolumeHandle(EFI_HANDLE ImageHandle)
{
	if(!ImageHandle) {
		lastStatus = EFI_INVALID_PARAMETER;
		return nullptr;
	}
	
	EFI_LOADED_IMAGE_PROTOCOL* loadedImage = nullptr;

	lastStatus = gSystem->BootServices->OpenProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&loadedImage, ImageHandle, nullptr, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	if(EFI_ERROR(lastStatus)) {
		printf("[BOOTMGR] [FileSystem] [ERROR]: HandleProtocol returned 0x%llX\r\n", lastStatus);
		return nullptr;
	}
	if(!loadedImage) {
		lastStatus = EFI_UNSUPPORTED;
		return nullptr;
	}

	if(!loadedImage->DeviceHandle) {
		lastStatus = EFI_DEVICE_ERROR;
		return nullptr;
	}

	return loadedImage->DeviceHandle;
}

EFI_FILE_PROTOCOL* FS::OpenVolume(EFI_HANDLE VolumeHandle)
{
	if(!VolumeHandle) {
		lastStatus = EFI_INVALID_PARAMETER;
		return nullptr;
	}

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = nullptr;
	lastStatus = gSystem->BootServices->HandleProtocol(VolumeHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&fs);

	if(EFI_ERROR(lastStatus)) return nullptr;
	if(!fs) {
		lastStatus = EFI_DEVICE_ERROR;
		return nullptr;
	}

	EFI_FILE_PROTOCOL* root = nullptr;
	lastStatus = fs->OpenVolume(fs, &root);
	return root;
}

EFI_FILE_PROTOCOL* FS::OpenFile(EFI_FILE_PROTOCOL* volume, const wchar_t* path, uint64_t openMode, uint64_t attribs)
{
	if(!volume || !path) {
		lastStatus = EFI_INVALID_PARAMETER;
		return nullptr;
	}

	EFI_FILE_PROTOCOL* out = nullptr;
	lastStatus = volume->Open(volume, &out, (CHAR16*)path, openMode, attribs);
	return out;
}

size_t FS::ReadFile(EFI_FILE_PROTOCOL* file, void* buffer, size_t length)
{
	if(!file || !buffer) {
		lastStatus = EFI_INVALID_PARAMETER;
		return 0;
	}

	uint64_t size = length;
	lastStatus = file->Read(file, &size, buffer);
	return size;
}

EFI_STATUS FS::Seek(EFI_FILE_PROTOCOL* file, size_t offset)
{
	if(!file) return EFI_INVALID_PARAMETER;

	return file->SetPosition(file, offset);
}