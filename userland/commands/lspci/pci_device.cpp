#include "pci_device.hpp"
#include <cstdio>

error_t output_target_device(char* buf, size_t len, int vendor_id, int device_id)
{
	switch (vendor_id) {
		case INTEL_VENDOR_ID: {
			const char* intel = "Intel Corporation.";
			switch (device_id) {
				case 0x1237:
					snprintf(buf, len, "%s 440FX - 82441FX PMC [Natoma]", intel);
					break;
				case 0x100e:
					snprintf(buf, len, "%s 82540EM Gigabit Ethernet Controller",
							 intel);
					break;
				case 0x7113:
					snprintf(buf, len, "%s 82371AB/EB/MB PIIX4 ACPI", intel);
					break;
				default:
					snprintf(buf, len, "%s Unknown device", intel);
					break;
			}

			break;
		}

		case QEMU_VENDOR_ID: {
			const char* qemu = "QEMU.";
			switch (device_id) {
				case 0x1111:
					snprintf(buf, len, "%s QEMU Virtual Machine", qemu);
					break;
				default:
					snprintf(buf, len, "%s Unknown device", qemu);
					break;
			}

			break;
		}

		case REDHAT_VENDOR_ID: {
			const char* redhat = "Red Hat, Inc.";
			switch (device_id) {
				case 0x1000:
					snprintf(buf, len, "%s Virtio network device", redhat);
					break;
				case 0x1001:
					snprintf(buf, len, "%s Virtio block device", redhat);
					break;
				case 0x1002:
					snprintf(buf, len, "%s Virtio console", redhat);
					break;
				case 0x1003:
					snprintf(buf, len, "%s Virtio SCSI", redhat);
					break;
				case 0x1004:
					snprintf(buf, len, "%s Virtio entropy source", redhat);
					break;
				case 0x1005:
					snprintf(buf, len, "%s Virtio balloon", redhat);
					break;
				case 0x1009:
					snprintf(buf, len, "%s Virtio filesystem", redhat);
					break;
				case 0x1010:
					snprintf(buf, len, "%s Virtio RNG", redhat);
					break;
				case 0x1011:
					snprintf(buf, len, "%s Virtio GPU", redhat);
					break;
				case 0x1012:
					snprintf(buf, len, "%s Virtio input device", redhat);
					break;
				case 0x1013:
					snprintf(buf, len, "%s Virtio video device", redhat);
					break;
				default:
					snprintf(buf, len, "%s Unknown device", redhat);
					break;
			}

			break;
		}

		case NEC_VENDOR_ID: {
			const char* nec = "NEC Corporation.";
			switch (device_id) {
				case 0x0194:
					snprintf(buf, len, "%s uPD720200 USB 3.0 Host Controller", nec);
					break;
				default:
					snprintf(buf, len, "%s Unknown device", nec);
					break;
			}

			break;
		}

		default:
			snprintf(buf, len, "Unknown device.");
			return -1;
	}

	return OK;
}
