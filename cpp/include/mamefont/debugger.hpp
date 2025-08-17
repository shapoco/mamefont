#pragma once

#ifdef MAMEFONT_DEBUG

#include <stdarg.h>
#include <stdio.h>

#include "mamefont/decoder_context.hpp"
#include "mamefont/glyph.hpp"
#include "mamefont/instruction_set.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

extern bool decoderVerbose;

class Debugger {
 public:
  Operator dbgLastOp = Operator::NONE;
  frag_index_t dbgDumpCursor;
  Operator *dbgOpLog = nullptr;
  prog_cntr_t dbgStartPc = 0;
  prog_cntr_t dbgLastPc = 0;

  int dbgNumInstsPerOpr[static_cast<int>(Operator::COUNT)] = {0};
  int dbgGenFragsPerOpr[static_cast<int>(Operator::COUNT)] = {0};

  void init(const Glyph *glyph, const DecoderContext &ctx);
  void logInstructionPerformance(Operator op, int size);
  void dbgBeforeFetch(const DecoderContext &ctx);
  void dbgAfterOp(const DecoderContext &ctx, uint8_t len);
  void dbgBeforeOp(const DecoderContext &ctx, Operator op, const char *fmt,
                   ...);

  static inline void setVerbose(bool verbose) { decoderVerbose = verbose; }
};

#ifdef MAMEFONT_INCLUDE_IMPL

bool decoderVerbose;

void Debugger::init(const Glyph *glyph, const DecoderContext &ctx) {
  if (decoderVerbose) {
    printf("----------------------------------------\n");
    printf("MameFont Decoder Debugger\n");
    printf("----------------------------------------\n");
    printf("entryPoint  : %d\n", (int)(ctx.pc - ctx.bytecode));
    printf("glyphWidth  : %d\n", glyph->glyphWidth);
    printf("glyphHeight : %d\n", glyph->glyphHeight);
    printf("yOffset     : %d\n", glyph->yOffset);
    printf("numTracks   : %d\n", ctx.numTracks);
    printf("trackLength : %d\n", ctx.trackLength);
  }
  dbgStartPc = dbgLastPc = (ctx.pc - ctx.bytecode);
  dbgDumpCursor = ctx.cursor;

  if (dbgOpLog) delete[] dbgOpLog;
  dbgOpLog = new Operator[ctx.numTracks * ctx.trackLength];

  memset(dbgOpLog, 0, ctx.numTracks * ctx.trackLength * sizeof(Operator));
  memset(dbgNumInstsPerOpr, 0, sizeof(dbgNumInstsPerOpr));
  memset(dbgGenFragsPerOpr, 0, sizeof(dbgGenFragsPerOpr));
}

void Debugger::logInstructionPerformance(Operator op, int size) {
  dbgNumInstsPerOpr[static_cast<int>(op)]++;
  dbgGenFragsPerOpr[static_cast<int>(op)] += size;
}

void Debugger::dbgBeforeFetch(const DecoderContext &ctx) {}

void Debugger::dbgBeforeOp(const DecoderContext &ctx, Operator op,
                           const char *fmt, ...) {
  va_list args;

  dbgLastOp = op;
  dbgDumpCursor = ctx.cursor;
  if (dbgDumpCursor < (ctx.numTracks * ctx.trackLength)) {
    dbgOpLog[dbgDumpCursor] = op;
  }

  if (!decoderVerbose) return;

  int inst_size = instSizeOf(op);

  char logBuff[128];
  char *logPtr = logBuff;
  char *logEnd = logBuff + sizeof(logBuff);
  logPtr += snprintf(logPtr, logEnd - logPtr, "%5d, %4d,",
                     dbgLastPc, (int)dbgDumpCursor);
  for (int i = 0; i < 4; i++) {
    if (i < (inst_size)) {
      logPtr += snprintf(logPtr, logEnd - logPtr, " %02X",
                         readBlobU8(ctx.bytecode + (dbgLastPc + i)));
    } else {
      logPtr += snprintf(logPtr, logEnd - logPtr, "   ");
    }
  }
  logPtr += snprintf(logPtr, logEnd - logPtr, ", %-4s", mnemonicOf(op));

  va_start(args, fmt);
  logPtr += vsnprintf(logPtr, logEnd - logPtr, fmt, args);
  va_end(args);

  printf("%-64s", logBuff);
  if (logPtr - logBuff > 64) {
    printf("\n");
    for (int i = 0; i < 64; i++) printf(" ");
  }
  printf("--> ");
  fflush(stdout);
}

void Debugger::dbgAfterOp(const DecoderContext &ctx, uint8_t len) {
  dbgLastPc = ctx.pc - ctx.bytecode;
  logInstructionPerformance(dbgLastOp, (len));

  if (!decoderVerbose) return;

  for (int i = 0; i < (len); i++) {
    if (i % 16 == 0 && i > 0) {
      for (int j = 0; j < 64 + 4; j++) {
        printf(" ");
      }
    }
    printf("%02X ", ctx.data[dbgDumpCursor++]);
    if ((i + 1) % 16 == 0 || (i + 1) == (len)) {
      printf("\n");
    }
  }
  fflush(stdout);
}
#endif

#define MAMEFONT_BEFORE_OP(dbg, ctx, op, fmt, ...) \
  (dbg).dbgBeforeOp(ctx, op, fmt, ##__VA_ARGS__)

#define MAMEFONT_AFTER_OP(dbg, ctx, len) (dbg).dbgAfterOp(ctx, len)

}  // namespace mamefont

#endif

#ifndef MAMEFONT_DEBUG

namespace mamefont {

using Debugger = uint8_t;

#define MAMEFONT_BEFORE_OP(dbg, ctx, op, fmt, ...) \
  do {                                             \
  } while (true)

#define MAMEFONT_AFTER_OP(dbg, ctx, len) \
  do {                                   \
  } while (true)

}  // namespace mamefont

#endif
