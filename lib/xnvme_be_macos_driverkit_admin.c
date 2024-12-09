// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <xnvme_be_macos_driverkit.h>
#include <mach/mach_error.h>
#include <errno.h>

#include <xnvme_dev.h>

int
xnvme_be_macos_driverkit_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)ctx->dev->be.state;
	uint64_t _buf_base_offset = 0;
	NvmeSubmitCmd cmd_return;
	struct buffer *buf;
	NvmeSubmitCmd cmd;
	size_t output_cnt;
	kern_return_t ret;

	if (dbuf) {
		buf = buffer_find(state->buffers, (uint64_t)dbuf);
		if (!buf) {
			XNVME_DEBUG("FAILED: buffer_find()");
			return -EINVAL;
		}
		_buf_base_offset = ((uint64_t)dbuf) - buf->vaddr;
	}

	cmd = (NvmeSubmitCmd){
		.queue_id = 0,
		.dbuf_nbytes = dbuf_nbytes,
		.dbuf_offset = _buf_base_offset,
		.dbuf_token = (uint64_t)dbuf - _buf_base_offset,
	};
	memcpy(cmd.cmd, &ctx->cmd, sizeof(struct xnvme_spec_cmd));
	cmd.backend_opaque = (uint64_t)ctx;

	output_cnt = sizeof(NvmeSubmitCmd);
	ret = IOConnectCallStructMethod(state->device_connection, NVME_ONESHOT, &cmd,
					sizeof(NvmeSubmitCmd), &cmd_return, &output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallStructMethod(NVME_ONESHOT); "
			    "0x%08x, '%s'",
			    ret, mach_error_string(ret));
		return -EIO;
	}

	memcpy(&ctx->cpl, cmd_return.cpl, sizeof(struct xnvme_spec_cpl));
	return 0;
}

#endif

struct xnvme_be_admin g_xnvme_be_macos_driverkit_admin = {
#ifdef XNVME_BE_MACOS_ENABLED
	.cmd_admin = xnvme_be_macos_driverkit_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
	.id = "driverkit",
};
