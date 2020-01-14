#include <linux/pci.h>
#include <linux/netdevice.h>
#include "dwc-xlgmac.h"


static int xlgmac_device_enable_sriov(struct pci_dev *pdev, int num_vfs)
{
    struct net_device *netdev  = pci_get_drvdata(pdev);
    struct xlgmac_pdata *pdata = netdev_priv(netdev);
    struct xlgmac_sriov *sriov = &pdata->sriov;
    int err;
    int vf;

    if (sriov->enabled_vfs) {
        dev_warn(pdev,
                   "failed to enable SRIOV on device, already enabled with %d vfs\n",
                   sriov->enabled_vfs);
        return -EBUSY;
    }

    for (vf = 0; vf < num_vfs; vf++) {
        sriov->vfs_ctx[vf].enabled = 1;
        sriov->enabled_vfs++;
        dev_warn(pdev, "successfully enabled VF* %d\n", vf);
    }

    return 0;
}

static void xlgmac_device_disable_sriov(struct pci_dev *pdev)
{
    struct net_device *netdev  = pci_get_drvdata(pdev);
    struct xlgmac_pdata *pdata = netdev_priv(netdev);
    struct xlgmac_sriov *sriov = &pdata->sriov;
    int err;
    int vf;

    for (vf = 0; vf < sriov->num_vfs; vf++) {
        if (!sriov->vfs_ctx[vf].enabled)
            continue;
        sriov->vfs_ctx[vf].enabled = 0;
        sriov->enabled_vfs--;
    }
}

static int xlgmac_pci_enable_sriov(struct pci_dev *pdev, int num_vfs)
{
    int err = 0;

    if (pci_num_vf(pdev)) {
        dev_warn(pdev, "Unable to enable pci sriov, already enabled\n");
        return -EBUSY;
    }

    err = pci_enable_sriov(pdev, num_vfs);
    if (err)
        dev_warn(pdev, "pci_enable_sriov failed : %d\n", err);

    return err;
}

static void xlgmac_pci_disable_sriov(struct pci_dev *pdev)
{
    pci_disable_sriov(pdev);
}

static int xlgmac_sriov_enable(struct pci_dev *pdev, int num_vfs)
{
    struct net_device *netdev  = pci_get_drvdata(pdev);
    struct xlgmac_pdata *pdata = netdev_priv(netdev);
    struct xlgmac_sriov *sriov = &pdata->sriov;
    int err = 0;

    err = xlgmac_device_enable_sriov(pdev, num_vfs);
    if (err) {
        dev_warn(pdev, "xlgmac_device_enable_sriov failed : %d\n", err);
        return err;
    }

    err = xlgmac_pci_enable_sriov(pdev, num_vfs);
    if (err) {
        dev_warn(pdev, "xlgmac_pci_enable_sriov failed : %d\n", err);
        xlgmac_device_disable_sriov(pdev);
        return err;
    }

    sriov->num_vfs = num_vfs;

    return 0;
}

static void xlgmac_sriov_disable(struct pci_dev *pdev)
{
    struct net_device *netdev  = pci_get_drvdata(pdev);
    struct xlgmac_pdata *pdata = netdev_priv(netdev);
    struct xlgmac_sriov *sriov = &pdata->sriov;

    xlgmac_pci_disable_sriov(pdev);
    xlgmac_device_disable_sriov(pdev);
    sriov->num_vfs = 0;
}

int xlgmac_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
    int err = 0;

    printk("requested num_vfs %d\n", num_vfs);
    if (!pdev->is_physfn)
        return -EPERM;

    if (num_vfs)
        err = xlgmac_sriov_enable(pdev, num_vfs);
    else
        xlgmac_sriov_disable(pdev);

    return err ? err : num_vfs;
}

int xlgmac_sriov_attach(struct pci_dev *pdev)
{
    struct net_device *netdev  = pci_get_drvdata(pdev);
    struct xlgmac_pdata *pdata = netdev_priv(netdev);
    struct xlgmac_sriov *sriov = &pdata->sriov;

    if (!pdev->is_physfn || !sriov->num_vfs)
        return 0;

    /* If sriov VFs exist in PCI level, enable them in device level */
    return xlgmac_device_enable_sriov(pdev, sriov->num_vfs);
}

void xlgmac_sriov_detach(struct pci_dev *pdev)
{
    if (!pdev->is_physfn)
        return;

    xlgmac_device_disable_sriov(pdev);
}

int xlgmac_sriov_init(struct pci_dev *pdev)
{
    struct net_device *netdev  = pci_get_drvdata(pdev);
    struct xlgmac_pdata *pdata = netdev_priv(netdev);
    struct xlgmac_sriov *sriov = &pdata->sriov;
    int total_vfs;

    if (!pdev->is_physfn)
        return 0;

    total_vfs = pci_sriov_get_totalvfs(pdev);
    sriov->num_vfs = pci_num_vf(pdev);
    sriov->vfs_ctx = kcalloc(total_vfs, sizeof(*sriov->vfs_ctx), GFP_KERNEL);
    if (!sriov->vfs_ctx)
        return -ENOMEM;

    return 0;
}

void xlgmac_sriov_cleanup(struct pci_dev *pdev)
{
    struct net_device *netdev  = pci_get_drvdata(pdev);
    struct xlgmac_pdata *pdata = netdev_priv(netdev);
    struct xlgmac_sriov *sriov = &pdata->sriov;

    if (!pdev->is_physfn)
        return;

    kfree(sriov->vfs_ctx);
}

int xlgmac_is_pf(struct pci_dev *pdev)
{
    return pdev->is_physfn;
}
