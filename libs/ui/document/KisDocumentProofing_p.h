/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_DOCUMENT_PROOFING_P_H
#define KIS_DOCUMENT_PROOFING_P_H

#include <kra/KisKraRasterCodec.h>
#include <kis_types.h>

namespace KisDocumentProofing {
KisProofingConfigurationSP read(const LibrePaint::ImportExport::Kra::ProofingMetadata &metadata);
LibrePaint::ImportExport::Kra::ProofingMetadata write(const KisProofingConfigurationSP &configuration,
    const std::optional<LibrePaint::ImportExport::Kra::ProofingMetadata> &retained);
}
#endif
