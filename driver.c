#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/gpio.h>

#define FC1100_VENDOR_ID 0x15EC
#define FC1100_DEVICE_ID 0x1100

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jared Foster @ UnitX Labs");
MODULE_DESCRIPTION("Driver for the Beckhoff FC1100 EtherCAT Slave Controller card");

static struct pci_device_id fc1100_ids[] = {
	{ PCI_DEVICE(FC1100_VENDOR_ID, FC1100_DEVICE_ID) },
	{ }
};
MODULE_DEVICE_TABLE(pci, fc1100_ids);

int read_device_config(struct pci_dev *pdev)
{
	u16 vendor, device, status_reg, command_reg;

	pci_read_config_word(pdev, FC1100_VENDOR_ID, &vendor);
	pci_read_config_word(pdev, FC1100_DEVICE_ID, &device);

	printk("[FC1100] Device vid: 0x%X  pid: 0x%X\n", vendor, device);

	pci_read_config_word(pdev, PCI_STATUS, &status_reg);

	printk("[FC1100] Device status reg: 0x%X\n", status_reg);


	pci_read_config_word(pdev, PCI_COMMAND, &command_reg);

	if (command_reg | PCI_COMMAND_MEMORY) {
		printk("[FC1100] Device supports memory access\n");

		return 0;
	}

	printk("[FC1100] Device doesn't supports memory access!");

	return -EIO;
}

static int fc1100_probe(struct pci_dev *dev, const struct pci_device_id *id) {
	int status;
	void __iomem *ptr_bar0;
	void __iomem *ptr_bar1;
	void __iomem *ptr_bar2;

	if (read_device_config(dev) < 0) {
		return -EIO;
	}

	status = pci_resource_len(dev, 0);
	printk("[FC1100] BAR0 is mapped to 0x%llx\n", pci_resource_start(dev, 0));

	status = pci_resource_len(dev, 1);
	printk("[FC1100] BAR1 is mapped to 0x%llx\n", pci_resource_start(dev, 1));

	status = pci_resource_len(dev, 2);
	printk("[FC1100] BAR2 is mapped to 0x%llx\n", pci_resource_start(dev, 2));

	status = pcim_enable_device(dev);
	if(status < 0) {
		printk("[FC1100] Could not enable device\n");
		return status;
	}

	status = pcim_iomap_regions(dev, BIT(0), KBUILD_MODNAME);
	if(status < 0) {
		printk("[FC1100] BAR0 is already in use!\n");
		return status;
	}
	ptr_bar0 = pcim_iomap_table(dev)[0];
	if(ptr_bar0 == NULL) {
		printk("[FC1100] BAR0 pointer is invalid\n");
		return -1;
	}

	status = pcim_iomap_regions(dev, BIT(1), KBUILD_MODNAME);
	if(status < 0) {
		printk("[FC1100] BAR1 is already in use!\n");
		return status;
	}
	ptr_bar1 = pcim_iomap_table(dev)[1];
	if(ptr_bar1 == NULL) {
		printk("[FC1100] BAR1 pointer is invalid\n");
		return -1;
	}

	status = pcim_iomap_regions(dev, BIT(2), KBUILD_MODNAME);
	if(status < 0) {
		printk("[FC1100] BAR2 is already in use!\n");
		return status;
	}
	ptr_bar2 = pcim_iomap_table(dev)[2];
	if(ptr_bar2 == NULL) {
		printk("[FC1100] BAR2 pointer is invalid\n");
		return -1;
	}

	iowrite8(0x23, ptr_bar2 + 0x1000);

	return 0;
}

static void fc1100_remove(struct pci_dev *dev) {
}

static struct pci_driver fc1100_driver = {
	.name = "fc1100",
	.id_table = fc1100_ids,
	.probe = fc1100_probe,
	.remove = fc1100_remove,
};

static int __init load_driver(void) {
	printk("[FC1100] Registering the PCI device\n");
	return pci_register_driver(&fc1100_driver);
}

static void __exit unload_driver(void) {
	printk("[FC1100] Unregistering the PCI device\n");
	pci_unregister_driver(&fc1100_driver);
}


module_init(load_driver);
module_exit(unload_driver);