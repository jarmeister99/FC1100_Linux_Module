#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>

#include "chardev.h"
#include "fc1100.h"

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jared Foster & Brian Frost @ UnitX Labs");
MODULE_DESCRIPTION("Driver for the Beckhoff FC1100 EtherCAT Slave Controller PCI card");

static void fc1100_remove(struct pci_dev *dev);
static int fc1100_probe(struct pci_dev *dev, const struct pci_device_id *id);

static struct pci_device_id fc1100_ids[] = {
	{ PCI_DEVICE(FC1100_VENDOR_ID, FC1100_DEVICE_ID) },
	{ }
};

MODULE_DEVICE_TABLE(pci, fc1100_ids);

static struct pci_driver fc1100_driver = {
	.name = "fc1100",
	.id_table = fc1100_ids,
	.probe = fc1100_probe,
	.remove = fc1100_remove,
};

void release_device(struct pci_dev *dev)
{
	pci_release_region(dev, 0);
	pci_release_region(dev, 1);
	pci_release_region(dev, 2);
	pci_disable_device(dev);
}

static int fc1100_probe(struct pci_dev *dev, const struct pci_device_id *id) {
	int status;
	unsigned long mmio_start2, mmio_len2;
	struct fc1100_driver *drv_priv;

	void __iomem *ptr_bar0;
	void __iomem *ptr_bar1;
	void __iomem *ptr_bar2;

	mmio_start2 = pci_resource_start(dev, 2);
	mmio_len2 = pci_resource_len(dev, 2);

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

	drv_priv = kzalloc(sizeof(struct fc1100_driver), GFP_KERNEL);
	if (!drv_priv) {
		release_device(dev);
		return -ENOMEM;
	}
	drv_priv->hwmem = ioremap(mmio_start2, mmio_len2);
	if (!drv_priv->hwmem) {
		release_device(dev);
		return -EIO;
	}

	create_char_devs(drv_priv);
	pci_set_drvdata(dev, drv_priv);

	return 0;
}

static void fc1100_remove(struct pci_dev *dev) {
	struct fc1100_driver *drv_priv = pci_get_drvdata(dev);
	destroy_char_devs();
	
	if (drv_priv) {
		if (drv_priv->hwmem) {
			iounmap(drv_priv->hwmem);
		}

		kfree(drv_priv);
	}

	release_device(dev);

	printk("[FC1100] Unloaded module");

}

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