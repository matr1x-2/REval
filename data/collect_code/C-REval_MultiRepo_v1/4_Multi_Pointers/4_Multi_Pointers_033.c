/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_033 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 23 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/firmware.h>
 * #include <linux/module.h>
 * #include <linux/pci.h>
 * #include <linux/reboot.h>
 * #include "amdgpu.h"
 * #include "amdgpu_smu.h"
 * #include "atomfirmware.h"
 * #include "amdgpu_atomfirmware.h"
 * #include "amdgpu_atombios.h"
 * #include "smu_v11_0.h"
 * #include "soc15_common.h"
 * #include "atom.h"
 * #include "amdgpu_ras.h"
 * #include "smu_cmn.h"
 * #include "asic_reg/thm/thm_11_0_2_offset.h"
 * #include "asic_reg/thm/thm_11_0_2_sh_mask.h"
 * #include "asic_reg/mp/mp_11_0_offset.h"
 * #include "asic_reg/mp/mp_11_0_sh_mask.h"
 * #include "asic_reg/smuio/smuio_11_0_0_offset.h"
 * #include "asic_reg/smuio/smuio_11_0_0_sh_mask.h"
 */
/* Context-Before
 * 	}
 * 
 * 	return ret;
 * }
 * 
 * static int smu_v11_0_set_pptable_v2_0(struct smu_context *smu, void **table, uint32_t *size)
 * {
 * 	struct amdgpu_device *adev = smu->adev;
 * 	uint32_t ppt_offset_bytes;
 * 	const struct smc_firmware_header_v2_0 *v2;
 * 
 * 	v2 = (const struct smc_firmware_header_v2_0 *) adev->pm.fw->data;
 * 
 * 	ppt_offset_bytes = le32_to_cpu(v2->ppt_offset_bytes);
 * 	*size = le32_to_cpu(v2->ppt_size_bytes);
 * 	*table = (uint8_t *)v2 + ppt_offset_bytes;
 * 
 * 	return 0;
 * }
 */
static int smu_v11_0_set_pptable_v2_1(struct smu_context *smu, void **table,
				      uint32_t *size, uint32_t pptable_id)
{
	struct amdgpu_device *adev = smu->adev;
	const struct smc_firmware_header_v2_1 *v2_1;
	struct smc_soft_pptable_entry *entries;
	uint32_t pptable_count = 0;
	int i = 0;

	v2_1 = (const struct smc_firmware_header_v2_1 *) adev->pm.fw->data;
	entries = (struct smc_soft_pptable_entry *)
		((uint8_t *)v2_1 + le32_to_cpu(v2_1->pptable_entry_offset));
	pptable_count = le32_to_cpu(v2_1->pptable_count);
	for (i = 0; i < pptable_count; i++) {
		if (le32_to_cpu(entries[i].id) == pptable_id) {
			*table = ((uint8_t *)v2_1 + le32_to_cpu(entries[i].ppt_offset_bytes));
			*size = le32_to_cpu(entries[i].ppt_size_bytes);
			break;
		}
	}

	if (i == pptable_count)
		return -EINVAL;

	return 0;
}
/* Context-After
 * int smu_v11_0_setup_pptable(struct smu_context *smu)
 * {
 * 	struct amdgpu_device *adev = smu->adev;
 * 	const struct smc_firmware_header_v1_0 *hdr;
 * 	int ret, index;
 * 	uint32_t size = 0;
 * 	uint16_t atom_table_size;
 * 	uint8_t frev, crev;
 * 	void *table;
 * 	uint16_t version_major, version_minor;
 * 
 * 	if (!amdgpu_sriov_vf(adev)) {
 * 		hdr = (const struct smc_firmware_header_v1_0 *) adev->pm.fw->data;
 * 		version_major = le16_to_cpu(hdr->header.header_version_major);
 * 		version_minor = le16_to_cpu(hdr->header.header_version_minor);
 * 		if (version_major == 2 && smu->smu_table.boot_values.pp_table_id > 0) {
 * 			dev_info(adev->dev, "use driver provided pptable %d\n", smu->smu_table.boot_values.pp_table_id);
 * 			switch (version_minor) {
 * 			case 0:
 */
