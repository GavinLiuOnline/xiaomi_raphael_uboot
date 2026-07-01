// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm SoC power off for phones.
 *
 * Mirrors the Linux msm-poweroff sequence: configure PMIC PON for shutdown,
 * disable the SPMI PMIC arbiter via TZ, then deassert PS_HOLD.
 */

#include <cpu_func.h>
#include <dm.h>
#include <irq_func.h>
#include <linux/arm-smccc.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <power/pmic.h>
#include <asm/io.h>
#include <mach/poweroff.h>

#define SCM_SVC_PWR			0x9
#define SCM_IO_DISABLE_PMIC_ARBITER	1
#define SCM_IO_DEASSERT_PS_HOLD		2
#define SCM_SIP_FNID(s, c) \
	(((((s) & 0xff) << 8) | ((c) & 0xff)) | 0x02000000)

#define PON_BASE			0x800
#define PON_PS_HOLD_RST_CTL		(PON_BASE + 0x5a)
#define PON_PS_HOLD_RST_CTL2		(PON_BASE + 0x5b)
#define PON_PS_HOLD_RESET_EN		BIT(7)
#define PON_PS_HOLD_TYPE_MASK		0xf
#define PON_POWER_OFF_SHUTDOWN		4

#define SM8150_PSHOLD_PHYS		0xc264000

static void qcom_scm_pmic_arbiter_disable(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SCM_SIP_FNID(SCM_SVC_PWR, SCM_IO_DISABLE_PMIC_ARBITER),
		      0, 0, 0, 0, 0, 0, 0, &res);
}

static void qcom_scm_ps_hold_deassert(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SCM_SIP_FNID(SCM_SVC_PWR, SCM_IO_DEASSERT_PS_HOLD),
		      0, 0, 0, 0, 0, 0, 0, &res);
}

static struct udevice *qcom_find_pm8150_pmic(void)
{
	struct udevice *dev;

	for (uclass_first_device(UCLASS_PMIC, &dev); dev;
	     uclass_next_device(&dev)) {
		if (device_is_compatible(dev, "qcom,pm8150"))
			return dev;
	}

	return NULL;
}

static int qcom_pon_configure_shutdown(struct udevice *pmic)
{
	int reg, ret;

	reg = pmic_reg_read(pmic, PON_PS_HOLD_RST_CTL2);
	if (reg < 0)
		return reg;

	reg &= ~PON_PS_HOLD_RESET_EN;
	ret = pmic_reg_write(pmic, PON_PS_HOLD_RST_CTL2, reg);
	if (ret)
		return ret;

	udelay(500);

	reg = pmic_reg_read(pmic, PON_PS_HOLD_RST_CTL);
	if (reg < 0)
		return reg;

	reg = (reg & ~PON_PS_HOLD_TYPE_MASK) | PON_POWER_OFF_SHUTDOWN;
	ret = pmic_reg_write(pmic, PON_PS_HOLD_RST_CTL, reg);
	if (ret)
		return ret;

	reg = pmic_reg_read(pmic, PON_PS_HOLD_RST_CTL2);
	if (reg < 0)
		return reg;

	reg |= PON_PS_HOLD_RESET_EN;
	return pmic_reg_write(pmic, PON_PS_HOLD_RST_CTL2, reg);
}

void qcom_soc_poweroff(void)
{
	struct udevice *pmic = qcom_find_pm8150_pmic();
	void __iomem *pshold;
	int ret;

	if (pmic) {
		ret = qcom_pon_configure_shutdown(pmic);
		if (ret)
			printf("qcom poweroff: PMIC cfg failed: %d\n", ret);
	} else {
		printf("qcom poweroff: PM8150 PMIC not found\n");
	}

	disable_interrupts();
	qcom_scm_pmic_arbiter_disable();

	pshold = map_physmem(SM8150_PSHOLD_PHYS, 4, MAP_NOCACHE);
	qcom_scm_ps_hold_deassert();
	if (pshold)
		writel(0, pshold);

	mdelay(10000);
}
