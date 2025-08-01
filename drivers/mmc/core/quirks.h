/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  This file contains work-arounds for many known SD/MMC
 *  and SDIO hardware bugs.
 *
 *  Copyright (c) 2011 Andrei Warkentin <andreiw@motorola.com>
 *  Copyright (c) 2011 Pierre Tardy <tardyp@gmail.com>
 *  Inspired from pci fixup code:
 *  Copyright (c) 1999 Martin Mares <mj@ucw.cz>
 *
 */

#include <linux/of.h>
#include <linux/mmc/sdio_ids.h>

#include "card.h"

static const struct mmc_fixup __maybe_unused mmc_sd_fixups[] = {
	/*
	 * Kingston Canvas Go! Plus microSD cards never finish SD cache flush.
	 * This has so far only been observed on cards from 11/2019, while new
	 * cards from 2023/05 do not exhibit this behavior.
	 */
	_FIXUP_EXT("SD64G", CID_MANFID_KINGSTON_SD, 0x5449, 2019, 11,
		   0, -1ull, SDIO_ANY_ID, SDIO_ANY_ID, add_quirk_sd,
		   MMC_QUIRK_BROKEN_SD_CACHE, EXT_CSD_REV_ANY),

	/*
	 * GIGASTONE Gaming Plus microSD cards manufactured on 02/2022 never
	 * clear Flush Cache bit and set Poweroff Notification Ready bit.
	 */
	_FIXUP_EXT("ASTC", CID_MANFID_GIGASTONE, 0x3456, 2022, 2,
		   0, -1ull, SDIO_ANY_ID, SDIO_ANY_ID, add_quirk_sd,
		   MMC_QUIRK_BROKEN_SD_CACHE | MMC_QUIRK_BROKEN_SD_POWEROFF_NOTIFY,
		   EXT_CSD_REV_ANY),

	/*
	 * Swissbit series S46-u cards throw I/O errors during tuning requests
	 * after the initial tuning request expectedly times out. This has
	 * only been observed on cards manufactured on 01/2019 that are using
	 * Bay Trail host controllers.
	 */
	_FIXUP_EXT("0016G", CID_MANFID_SWISSBIT, 0x5342, 2019, 1,
		   0, -1ull, SDIO_ANY_ID, SDIO_ANY_ID, add_quirk_sd,
		   MMC_QUIRK_NO_UHS_DDR50_TUNING, EXT_CSD_REV_ANY),

	/*
	 * Some SD cards reports discard support while they don't
	 */
	MMC_FIXUP(CID_NAME_ANY, CID_MANFID_SANDISK_SD, 0x5344, add_quirk_sd,
		  MMC_QUIRK_BROKEN_SD_DISCARD),

	END_FIXUP
};

static const struct mmc_fixup __maybe_unused mmc_blk_fixups[] = {
#define INAND_CMD38_ARG_EXT_CSD  113
#define INAND_CMD38_ARG_ERASE    0x00
#define INAND_CMD38_ARG_TRIM     0x01
#define INAND_CMD38_ARG_SECERASE 0x80
#define INAND_CMD38_ARG_SECTRIM1 0x81
#define INAND_CMD38_ARG_SECTRIM2 0x88
	/* CMD38 argument is passed through EXT_CSD[113] */
	MMC_FIXUP("SEM02G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM04G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM08G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM16G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM32G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),

	/*
	 * Some MMC cards experience performance degradation with CMD23
	 * instead of CMD12-bounded multiblock transfers. For now we'll
	 * black list what's bad...
	 * - Certain Toshiba cards.
	 *
	 * N.B. This doesn't affect SD cards.
	 */
	MMC_FIXUP("SDMB-32", CID_MANFID_SANDISK, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("SDM032", CID_MANFID_SANDISK, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC08G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC16G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC32G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),

	/*
	 * Some SD cards lockup while using CMD23 multiblock transfers.
	 */
	MMC_FIXUP("AF SD", CID_MANFID_ATP, CID_OEMID_ANY, add_quirk_sd,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("APUSD", CID_MANFID_APACER, 0x5048, add_quirk_sd,
		  MMC_QUIRK_BLK_NO_CMD23),

	/*
	 * Some MMC cards need longer data read timeout than indicated in CSD.
	 */
	MMC_FIXUP(CID_NAME_ANY, CID_MANFID_MICRON, 0x200, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),
	MMC_FIXUP("008GE0", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),

	/*
	 * On these Samsung MoviNAND parts, performing secure erase or
	 * secure trim can result in unrecoverable corruption due to a
	 * firmware bug.
	 */
	MMC_FIXUP("M8G2FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MAG4FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MBG8FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MCGAFA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VAL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("KYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VZL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),

	/*
	 *  On Some Kingston eMMCs, performing trim can result in
	 *  unrecoverable data conrruption occasionally due to a firmware bug.
	 */
	MMC_FIXUP("V10008", CID_MANFID_KINGSTON, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_TRIM_BROKEN),
	MMC_FIXUP("V10016", CID_MANFID_KINGSTON, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_TRIM_BROKEN),

	/*
	 * Kingston EMMC04G-M627 advertises TRIM but it does not seems to
	 * support being used to offload WRITE_ZEROES.
	 */
	MMC_FIXUP("M62704", CID_MANFID_KINGSTON, 0x0100, add_quirk_mmc,
		  MMC_QUIRK_TRIM_BROKEN),

	/*
	 * Micron MTFC4GACAJCN-1M supports TRIM but does not appear to support
	 * WRITE_ZEROES offloading. It also supports caching, but the cache can
	 * only be flushed after a write has occurred.
	 */
	MMC_FIXUP("Q2J54A", CID_MANFID_MICRON, 0x014e, add_quirk_mmc,
		  MMC_QUIRK_TRIM_BROKEN | MMC_QUIRK_BROKEN_CACHE_FLUSH),

	END_FIXUP
};

static const struct mmc_fixup __maybe_unused mmc_ext_csd_fixups[] = {
	/*
	 * Certain Hynix eMMC 4.41 cards might get broken when HPI feature
	 * is used so disable the HPI feature for such buggy cards.
	 */
	MMC_FIXUP_EXT_CSD_REV(CID_NAME_ANY, CID_MANFID_HYNIX,
			      0x014a, add_quirk, MMC_QUIRK_BROKEN_HPI, 5),
	/*
	 * Certain Micron (Numonyx) eMMC 4.5 cards might get broken when HPI
	 * feature is used so disable the HPI feature for such buggy cards.
	 */
	MMC_FIXUP_EXT_CSD_REV(CID_NAME_ANY, CID_MANFID_NUMONYX,
			      0x014e, add_quirk, MMC_QUIRK_BROKEN_HPI, 6),

	END_FIXUP
};


static const struct mmc_fixup __maybe_unused sdio_fixup_methods[] = {
	SDIO_FIXUP(SDIO_VENDOR_ID_TI_WL1251, SDIO_DEVICE_ID_TI_WL1251,
		   add_quirk, MMC_QUIRK_NONSTD_FUNC_IF),

	SDIO_FIXUP(SDIO_VENDOR_ID_TI_WL1251, SDIO_DEVICE_ID_TI_WL1251,
		   add_quirk, MMC_QUIRK_DISABLE_CD),

	SDIO_FIXUP(SDIO_VENDOR_ID_TI, SDIO_DEVICE_ID_TI_WL1271,
		   add_quirk, MMC_QUIRK_NONSTD_FUNC_IF),

	SDIO_FIXUP(SDIO_VENDOR_ID_TI, SDIO_DEVICE_ID_TI_WL1271,
		   add_quirk, MMC_QUIRK_DISABLE_CD),

	SDIO_FIXUP(SDIO_VENDOR_ID_STE, SDIO_DEVICE_ID_STE_CW1200,
		   add_quirk, MMC_QUIRK_BROKEN_BYTE_MODE_512),

	SDIO_FIXUP(SDIO_VENDOR_ID_MARVELL, SDIO_DEVICE_ID_MARVELL_8797_F0,
		   add_quirk, MMC_QUIRK_BROKEN_IRQ_POLLING),

	SDIO_FIXUP(SDIO_VENDOR_ID_MARVELL, SDIO_DEVICE_ID_MARVELL_8887_F0,
		   add_limit_rate_quirk, 150000000),

	END_FIXUP
};

static const struct mmc_fixup __maybe_unused sdio_card_init_methods[] = {
	SDIO_FIXUP_COMPATIBLE("ti,wl1251", wl1251_quirk, 0),

	SDIO_FIXUP_COMPATIBLE("silabs,wf200", add_quirk,
			      MMC_QUIRK_BROKEN_BYTE_MODE_512 |
			      MMC_QUIRK_LENIENT_FN0 |
			      MMC_QUIRK_BLKSZ_FOR_BYTE_MODE),

	END_FIXUP
};

static inline bool mmc_fixup_of_compatible_match(struct mmc_card *card,
						 const char *compatible)
{
	struct device_node *np;

	for_each_child_of_node(mmc_dev(card->host)->of_node, np) {
		if (of_device_is_compatible(np, compatible)) {
			of_node_put(np);
			return true;
		}
	}

	return false;
}

static inline void mmc_fixup_device(struct mmc_card *card,
				    const struct mmc_fixup *table)
{
	const struct mmc_fixup *f;
	u64 rev = cid_rev_card(card);

	for (f = table; f->vendor_fixup; f++) {
		if (f->manfid != CID_MANFID_ANY &&
		    f->manfid != card->cid.manfid)
			continue;
		if (f->oemid != CID_OEMID_ANY &&
		    f->oemid != card->cid.oemid)
			continue;
		if (f->name != CID_NAME_ANY &&
		    strncmp(f->name, card->cid.prod_name,
			    sizeof(card->cid.prod_name)))
			continue;
		if (f->cis_vendor != (u16)SDIO_ANY_ID &&
		    f->cis_vendor != card->cis.vendor)
			continue;
		if (f->cis_device != (u16)SDIO_ANY_ID &&
		    f->cis_device != card->cis.device)
			continue;
		if (f->ext_csd_rev != EXT_CSD_REV_ANY &&
		    f->ext_csd_rev != card->ext_csd.rev)
			continue;
		if (rev < f->rev_start || rev > f->rev_end)
			continue;
		if (f->of_compatible &&
		    !mmc_fixup_of_compatible_match(card, f->of_compatible))
			continue;
		if (f->year != CID_YEAR_ANY && f->year != card->cid.year)
			continue;
		if (f->month != CID_MONTH_ANY && f->month != card->cid.month)
			continue;

		dev_dbg(&card->dev, "calling %ps\n", f->vendor_fixup);
		f->vendor_fixup(card, f->data);
	}
}
