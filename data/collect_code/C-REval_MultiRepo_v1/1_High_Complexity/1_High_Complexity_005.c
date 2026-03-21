/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_005 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 21 */
/* NLOC: 117 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/blkdev.h>
 * #include <linux/pci.h>
 * #include <linux/interrupt.h>
 * #include <linux/slab.h>
 * #include <linux/utsname.h>
 * #include <scsi/scsi.h>
 * #include <scsi/scsi_device.h>
 * #include <scsi/scsi_host.h>
 * #include <scsi/scsi_transport_fc.h>
 * #include <scsi/fc/fc_fs.h>
 * #include "lpfc_hw4.h"
 * #include "lpfc_hw.h"
 * #include "lpfc_sli.h"
 * #include "lpfc_sli4.h"
 * #include "lpfc_nl.h"
 * #include "lpfc_disc.h"
 * #include "lpfc.h"
 * #include "lpfc_scsi.h"
 * #include "lpfc_logmsg.h"
 * #include "lpfc_crtn.h"
 */
/* Context-Before
 * 		 * current driver state.
 * 		 */
 * 		if (vport->port_state >= LPFC_DISC_AUTH) {
 * 			if (test_bit(FC_RSCN_MODE, &vport->fc_flag)) {
 * 				lpfc_els_flush_rscn(vport);
 * 				/* RSCN still */
 * 				set_bit(FC_RSCN_MODE, &vport->fc_flag);
 * 			} else {
 * 				lpfc_els_flush_rscn(vport);
 * 			}
 * 		}
 * 
 * 		lpfc_disc_start(vport);
 * 	}
 * out:
 * 	lpfc_ct_free_iocb(phba, cmdiocb);
 * 	lpfc_nlp_put(ndlp);
 * }
 * 
 * static void
 */
lpfc_cmpl_ct_cmd_gff_id(struct lpfc_hba *phba, struct lpfc_iocbq *cmdiocb,
			struct lpfc_iocbq *rspiocb)
{
	struct lpfc_vport *vport = cmdiocb->vport;
	struct lpfc_dmabuf *inp = cmdiocb->cmd_dmabuf;
	struct lpfc_dmabuf *outp = cmdiocb->rsp_dmabuf;
	struct lpfc_sli_ct_request *CTrsp;
	int did, rc, retry;
	uint8_t fbits;
	struct lpfc_nodelist *ndlp = NULL, *free_ndlp = NULL;
	u32 ulp_status = get_job_ulpstatus(phba, rspiocb);
	u32 ulp_word4 = get_job_word4(phba, rspiocb);

	did = ((struct lpfc_sli_ct_request *) inp->virt)->un.gff.PortId;
	did = be32_to_cpu(did);

	lpfc_debugfs_disc_trc(vport, LPFC_DISC_TRC_CT,
		"GFF_ID cmpl:     status:x%x/x%x did:x%x",
		ulp_status, ulp_word4, did);

	/* Ignore response if link flipped after this request was made */
	if (cmdiocb->event_tag != phba->fc_eventTag) {
		lpfc_printf_vlog(vport, KERN_INFO, LOG_DISCOVERY,
				 "9045 Event tag mismatch. Ignoring NS rsp\n");
		goto iocb_free;
	}

	if (ulp_status == IOSTAT_SUCCESS) {
		/* Good status, continue checking */
		CTrsp = (struct lpfc_sli_ct_request *) outp->virt;
		fbits = CTrsp->un.gff_acc.fbits[FCP_TYPE_FEATURE_OFFSET];

		lpfc_printf_vlog(vport, KERN_INFO, LOG_DISCOVERY,
				 "6431 Process GFF_ID rsp for %08x "
				 "fbits %02x %s %s\n",
				 did, fbits,
				 (fbits & FC4_FEATURE_INIT) ? "Initiator" : " ",
				 (fbits & FC4_FEATURE_TARGET) ? "Target" : " ");

		if (be16_to_cpu(CTrsp->CommandResponse.bits.CmdRsp) ==
		    SLI_CT_RESPONSE_FS_ACC) {
			if ((fbits & FC4_FEATURE_INIT) &&
			    !(fbits & FC4_FEATURE_TARGET)) {
				lpfc_printf_vlog(vport, KERN_INFO,
						 LOG_DISCOVERY,
						 "0270 Skip x%x GFF "
						 "NameServer Rsp Data: (init) "
						 "x%x x%x\n", did, fbits,
						 vport->fc_rscn_id_cnt);
				goto out;
			}
		}
	}
	else {
		/* Check for retry */
		if (cmdiocb->retry < LPFC_MAX_NS_RETRY) {
			retry = 1;
			if (ulp_status == IOSTAT_LOCAL_REJECT) {
				switch ((ulp_word4 &
					IOERR_PARAM_MASK)) {

				case IOERR_NO_RESOURCES:
					/* We don't increment the retry
					 * count for this case.
					 */
					break;
				case IOERR_LINK_DOWN:
				case IOERR_SLI_ABORTED:
				case IOERR_SLI_DOWN:
					retry = 0;
					break;
				default:
					cmdiocb->retry++;
				}
			}
			else
				cmdiocb->retry++;

			if (retry) {
				/* CT command is being retried */
				rc = lpfc_ns_cmd(vport, SLI_CTNS_GFF_ID,
					 cmdiocb->retry, did);
				if (rc == 0) {
					/* success */
					free_ndlp = cmdiocb->ndlp;
					lpfc_ct_free_iocb(phba, cmdiocb);
					lpfc_nlp_put(free_ndlp);
					return;
				}
			}
		}
		lpfc_printf_vlog(vport, KERN_ERR, LOG_TRACE_EVENT,
				 "0267 NameServer GFF Rsp "
				 "x%x Error (%d %d) Data: x%lx x%x\n",
				 did, ulp_status, ulp_word4,
				 vport->fc_flag, vport->fc_rscn_id_cnt);
	}

	/* This is a target port, unregistered port, or the GFF_ID failed */
	ndlp = lpfc_setup_disc_node(vport, did);
	if (ndlp) {
		lpfc_printf_vlog(vport, KERN_INFO, LOG_DISCOVERY,
				 "0242 Process x%x GFF "
				 "NameServer Rsp Data: x%lx x%lx x%x\n",
				 did, ndlp->nlp_flag, vport->fc_flag,
				 vport->fc_rscn_id_cnt);
	} else {
		lpfc_printf_vlog(vport, KERN_INFO, LOG_DISCOVERY,
				 "0243 Skip x%x GFF "
				 "NameServer Rsp Data: x%lx x%x\n", did,
				 vport->fc_flag, vport->fc_rscn_id_cnt);
	}
out:
	/* Link up / RSCN discovery */
	if (vport->num_disc_nodes)
		vport->num_disc_nodes--;

	lpfc_printf_vlog(vport, KERN_INFO, LOG_DISCOVERY,
			 "6451 GFF_ID cmpl inp %d disc %d\n",
			 vport->gidft_inp, vport->num_disc_nodes);

	if (vport->num_disc_nodes == 0) {
		/*
		 * The driver has cycled through all Nports in the RSCN payload.
		 * Complete the handling by cleaning up and marking the
		 * current driver state.
		 */
		if (vport->port_state >= LPFC_DISC_AUTH) {
			if (test_bit(FC_RSCN_MODE, &vport->fc_flag)) {
				lpfc_els_flush_rscn(vport);
				/* RSCN still */
				set_bit(FC_RSCN_MODE, &vport->fc_flag);
			} else {
				lpfc_els_flush_rscn(vport);
			}
		}
		lpfc_disc_start(vport);
	}

iocb_free:
	free_ndlp = cmdiocb->ndlp;
	lpfc_ct_free_iocb(phba, cmdiocb);
	lpfc_nlp_put(free_ndlp);
	return;
}
/* Context-After
 * static void
 * lpfc_cmpl_ct_cmd_gft_id(struct lpfc_hba *phba, struct lpfc_iocbq *cmdiocb,
 * 			struct lpfc_iocbq *rspiocb)
 * {
 * 	struct lpfc_vport *vport = cmdiocb->vport;
 * 	struct lpfc_dmabuf *inp = cmdiocb->cmd_dmabuf;
 * 	struct lpfc_dmabuf *outp = cmdiocb->rsp_dmabuf;
 * 	struct lpfc_sli_ct_request *CTrsp;
 * 	int did;
 * 	struct lpfc_nodelist *ndlp = NULL;
 * 	struct lpfc_nodelist *ns_ndlp = cmdiocb->ndlp;
 * 	uint32_t fc4_data_0, fc4_data_1;
 * 	u32 ulp_status = get_job_ulpstatus(phba, rspiocb);
 * 	u32 ulp_word4 = get_job_word4(phba, rspiocb);
 * 
 * 	did = ((struct lpfc_sli_ct_request *)inp->virt)->un.gft.PortId;
 * 	did = be32_to_cpu(did);
 * 
 * 	lpfc_debugfs_disc_trc(vport, LPFC_DISC_TRC_CT,
 */
