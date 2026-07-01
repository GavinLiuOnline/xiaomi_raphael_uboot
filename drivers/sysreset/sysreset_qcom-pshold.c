// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm PSHOLD reset driver
 *
 * Copyright (c) 2024 Sartura Ltd.
 *
 * Author: Robert Marko <robert.marko@sartura.hr>
 * Based on the Linux msm-poweroff driver.
 *
 */

#include <dm.h>
#include <sysreset.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <mach/poweroff.h>

struct qcom_pshold_priv {
	void __iomem *iomem;
};

static int qcom_pshold_request(struct udevice *dev, enum sysreset_t type)
{
	struct qcom_pshold_priv *priv = dev_get_priv(dev);

	if (type == SYSRESET_POWER_OFF) {
		qcom_soc_poweroff();
		return -EINPROGRESS;
	}

	if (!priv->iomem)
		return -ENODEV;

	writel(0, priv->iomem);
	mdelay(10000);

	return -EINPROGRESS;
}

static struct sysreset_ops qcom_pshold_ops = {
	.request = qcom_pshold_request,
};

static int qcom_pshold_probe(struct udevice *dev)
{
	struct qcom_pshold_priv *priv = dev_get_priv(dev);

	priv->iomem = dev_read_addr_name_ptr(dev, "pshold-base");
	if (!priv->iomem)
		priv->iomem = map_physmem(0xc264000, 4, MAP_NOCACHE);

	return priv->iomem ? 0 : -EINVAL;
}

static const struct udevice_id qcom_pshold_ids[] = {
	{ .compatible = "qcom,pshold", },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(qcom_pshold) = {
	.name		= "qcom_pshold",
	.id		= UCLASS_SYSRESET,
	.of_match	= qcom_pshold_ids,
	.probe		= qcom_pshold_probe,
	.priv_auto	= sizeof(struct qcom_pshold_priv),
	.ops		= &qcom_pshold_ops,
};
