// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mem_pass.h"

#include "basic_block.h"
#include "cfa.h"
#include "ir_context.h"
#include "iterator.h"

namespace spvtools {
namespace opt {

namespace {

const uint32_t kAccessChainPtrIdInIdx = 0;
const uint32_t kCopyObjectOperandInIdx = 0;
const uint32_t kLoadPtrIdInIdx = 0;
const uint32_t kLoopMergeMergeBlockIdInIdx = 0;
const uint32_t kStorePtrIdInIdx = 0;
const uint32_t kStoreValIdInIdx = 1;
const uint32_t kTypePointerStorageClassInIdx = 0;
const uint32_t kTypePointerTypeIdInIdx = 1;

}  // namespace

bool MemPass::IsBaseTargetType(const ir::Instruction* typeInst) const {
  switch (typeInst->opcode()) {
    case SpvOpTypeInt:
    case SpvOpTypeFloat:
    case SpvOpTypeBool:
    case SpvOpTypeVector:
    case SpvOpTypeMatrix:
    case SpvOpTypeImage:
    case SpvOpTypeSampler:
    case SpvOpTypeSampledImage:
      return true;
    default:
      break;
  }
  return false;
}

bool MemPass::IsTargetType(const ir::Instruction* typeInst) const {
  if (IsBaseTargetType(typeInst)) return true;
  if (typeInst->opcode() == SpvOpTypeArray)
    return IsBaseTargetType(
        get_def_use_mgr()->GetDef(typeInst->GetSingleWordOperand(1)));
  if (typeInst->opcode() != SpvOpTypeStruct) return false;
  // All struct members must be math type
  int nonMathComp = 0;
  typeInst->ForEachInId([&nonMathComp, this](const uint32_t* tid) {
    ir::Instruction* compTypeInst = get_def_use_mgr()->GetDef(*tid);
    if (!IsTargetType(compTypeInst)) ++nonMathComp;
  });
  return nonMathComp == 0;
}

bool MemPass::IsNonPtrAccessChain(const SpvOp opcode) const {
  return opcode == SpvOpAccessChain || opcode == SpvOpInBoundsAccessChain;
}

bool MemPass::IsPtr(uint32_t ptrId) {
  uint32_t varId = ptrId;
  ir::Instruction* ptrInst = get_def_use_mgr()->GetDef(varId);
  while (ptrInst->opcode() == SpvOpCopyObject) {
    varId = ptrInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    ptrInst = get_def_use_mgr()->GetDef(varId);
  }
  const SpvOp op = ptrInst->opcode();
  if (op == SpvOpVariable || IsNonPtrAccessChain(op)) return true;
  if (op != SpvOpFunctionParameter) return false;
  const uint32_t varTypeId = ptrInst->type_id();
  const ir::Instruction* varTypeInst = get_def_use_mgr()->GetDef(varTypeId);
  return varTypeInst->opcode() == SpvOpTypePointer;
}

ir::Instruction* MemPass::GetPtr(uint32_t ptrId, uint32_t* varId) {
  *varId = ptrId;
  ir::Instruction* ptrInst = get_def_use_mgr()->GetDef(*varId);
  while (ptrInst->opcode() == SpvOpCopyObject) {
    *varId = ptrInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    ptrInst = get_def_use_mgr()->GetDef(*varId);
  }
  ir::Instruction* varInst = ptrInst;
  while (varInst->opcode() != SpvOpVariable &&
         varInst->opcode() != SpvOpFunctionParameter) {
    if (IsNonPtrAccessChain(varInst->opcode())) {
      *varId = varInst->GetSingleWordInOperand(kAccessChainPtrIdInIdx);
    } else {
      assert(varInst->opcode() == SpvOpCopyObject);
      *varId = varInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    }
    varInst = get_def_use_mgr()->GetDef(*varId);
  }
  return ptrInst;
}

ir::Instruction* MemPass::GetPtr(ir::Instruction* ip, uint32_t* varId) {
  const SpvOp op = ip->opcode();
  assert(op == SpvOpStore || op == SpvOpLoad);
  const uint32_t ptrId = ip->GetSingleWordInOperand(
      op == SpvOpStore ? kStorePtrIdInIdx : kLoadPtrIdInIdx);
  return GetPtr(ptrId, varId);
}

bool MemPass::HasOnlyNamesAndDecorates(uint32_t id) const {
  bool hasOnlyNamesAndDecorates = true;
  get_def_use_mgr()->ForEachUser(
      id, [this, &hasOnlyNamesAndDecorates](ir::Instruction* user) {
        SpvOp op = user->opcode();
        if (op != SpvOpName && !IsNonTypeDecorate(op)) {
          hasOnlyNamesAndDecorates = false;
        }
      });
  return hasOnlyNamesAndDecorates;
}

void MemPass::KillAllInsts(ir::BasicBlock* bp) {
  bp->ForEachInst([this](ir::Instruction* ip) { context()->KillInst(ip); });
}

bool MemPass::HasLoads(uint32_t varId) const {
  bool hasLoads = false;
  get_def_use_mgr()->ForEachUser(varId, [this,
                                         &hasLoads](ir::Instruction* user) {
    SpvOp op = user->opcode();
    // TODO(): The following is slightly conservative. Could be
    // better handling of non-store/name.
    if (IsNonPtrAccessChain(op) || op == SpvOpCopyObject) {
      if (HasLoads(user->result_id())) {
        hasLoads = true;
      }
    } else if (op != SpvOpStore && op != SpvOpName && !IsNonTypeDecorate(op)) {
      hasLoads = true;
    }
  });
  return hasLoads;
}

bool MemPass::IsLiveVar(uint32_t varId) const {
  const ir::Instruction* varInst = get_def_use_mgr()->GetDef(varId);
  // assume live if not a variable eg. function parameter
  if (varInst->opcode() != SpvOpVariable) return true;
  // non-function scope vars are live
  const uint32_t varTypeId = varInst->type_id();
  const ir::Instruction* varTypeInst = get_def_use_mgr()->GetDef(varTypeId);
  if (varTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx) !=
      SpvStorageClassFunction)
    return true;
  // test if variable is loaded from
  return HasLoads(varId);
}

bool MemPass::IsLiveStore(ir::Instruction* storeInst) {
  // get store's variable
  uint32_t varId;
  (void)GetPtr(storeInst, &varId);
  return IsLiveVar(varId);
}

void MemPass::AddStores(uint32_t ptr_id, std::queue<ir::Instruction*>* insts) {
  get_def_use_mgr()->ForEachUser(ptr_id, [this, insts](ir::Instruction* user) {
    SpvOp op = user->opcode();
    if (IsNonPtrAccessChain(op)) {
      AddStores(user->result_id(), insts);
    } else if (op == SpvOpStore) {
      insts->push(user);
    }
  });
}

void MemPass::DCEInst(ir::Instruction* inst) {
  std::queue<ir::Instruction*> deadInsts;
  deadInsts.push(inst);
  while (!deadInsts.empty()) {
    ir::Instruction* di = deadInsts.front();
    // Don't delete labels
    if (di->opcode() == SpvOpLabel) {
      deadInsts.pop();
      continue;
    }
    // Remember operands
    std::vector<uint32_t> ids;
    di->ForEachInId([&ids](uint32_t* iid) { ids.push_back(*iid); });
    uint32_t varId = 0;
    // Remember variable if dead load
    if (di->opcode() == SpvOpLoad) (void)GetPtr(di, &varId);
    context()->KillInst(di);
    // For all operands with no remaining uses, add their instruction
    // to the dead instruction queue.
    for (auto id : ids)
      if (HasOnlyNamesAndDecorates(id))
        deadInsts.push(get_def_use_mgr()->GetDef(id));
    // if a load was deleted and it was the variable's
    // last load, add all its stores to dead queue
    if (varId != 0 && !IsLiveVar(varId)) AddStores(varId, &deadInsts);
    deadInsts.pop();
  }
}

void MemPass::ReplaceAndDeleteLoad(ir::Instruction* loadInst, uint32_t replId) {
  const uint32_t loadId = loadInst->result_id();
  context()->KillNamesAndDecorates(loadId);
  (void)context()->ReplaceAllUsesWith(loadId, replId);
  DCEInst(loadInst);
}

MemPass::MemPass() {}

bool MemPass::HasOnlySupportedRefs(uint32_t varId) {
  if (supported_ref_vars_.find(varId) != supported_ref_vars_.end()) return true;
  bool hasOnlySupportedRefs = true;
  get_def_use_mgr()->ForEachUser(
      varId, [this, &hasOnlySupportedRefs](ir::Instruction* user) {
        SpvOp op = user->opcode();
        if (op != SpvOpStore && op != SpvOpLoad && op != SpvOpName &&
            !IsNonTypeDecorate(op)) {
          hasOnlySupportedRefs = false;
        }
      });
  return hasOnlySupportedRefs;
}

void MemPass::InitSSARewrite(ir::Function* func) {
  // Clear collections.
  seen_target_vars_.clear();
  seen_non_target_vars_.clear();
  visitedBlocks_.clear();
  type2undefs_.clear();
  supported_ref_vars_.clear();
  label2ssa_map_.clear();
  phis_to_patch_.clear();

  // Collect target (and non-) variable sets. Remove variables with
  // non-load/store refs from target variable set
  for (auto& blk : *func) {
    for (auto& inst : blk) {
      switch (inst.opcode()) {
        case SpvOpStore:
        case SpvOpLoad: {
          uint32_t varId;
          (void)GetPtr(&inst, &varId);
          if (!IsTargetVar(varId)) break;
          if (HasOnlySupportedRefs(varId)) break;
          seen_non_target_vars_.insert(varId);
          seen_target_vars_.erase(varId);
        } break;
        default:
          break;
      }
    }
  }
}

bool MemPass::IsLiveAfter(uint32_t var_id, uint32_t label) const {
  // For now, return very conservative result: true. This will result in
  // correct, but possibly usused, phi code to be generated. A subsequent
  // DCE pass should eliminate this code.
  // TODO(greg-lunarg): Return more accurate information
  (void)var_id;
  (void)label;
  return true;
}

void MemPass::SSABlockInitSinglePred(ir::BasicBlock* block_ptr) {
  // Copy map entry from single predecessor
  const uint32_t label = block_ptr->id();
  const uint32_t predLabel = cfg()->preds(label).front();
  assert(visitedBlocks_.find(predLabel) != visitedBlocks_.end());
  label2ssa_map_[label] = label2ssa_map_[predLabel];
}

uint32_t MemPass::Type2Undef(uint32_t type_id) {
  const auto uitr = type2undefs_.find(type_id);
  if (uitr != type2undefs_.end()) return uitr->second;
  const uint32_t undefId = TakeNextId();
  std::unique_ptr<ir::Instruction> undef_inst(
      new ir::Instruction(context(), SpvOpUndef, type_id, undefId, {}));
  get_def_use_mgr()->AnalyzeInstDefUse(&*undef_inst);
  get_module()->AddGlobalValue(std::move(undef_inst));
  type2undefs_[type_id] = undefId;
  return undefId;
}

void MemPass::SSABlockInitLoopHeader(
    std::list<ir::BasicBlock*>::iterator block_itr) {
  const uint32_t label = (*block_itr)->id();

  // Determine the back-edge label.
  uint32_t backLabel = 0;
  for (uint32_t predLabel : cfg()->preds(label))
    if (visitedBlocks_.find(predLabel) == visitedBlocks_.end()) {
      assert(backLabel == 0);
      backLabel = predLabel;
      break;
    }
  assert(backLabel != 0);

  // Determine merge block.
  auto mergeInst = (*block_itr)->end();
  --mergeInst;
  --mergeInst;
  uint32_t mergeLabel =
      mergeInst->GetSingleWordInOperand(kLoopMergeMergeBlockIdInIdx);

  // Collect all live variables and a default value for each across all
  // non-backedge predecesors. Must be ordered map because phis are
  // generated based on order and test results will otherwise vary across
  // platforms.
  std::map<uint32_t, uint32_t> liveVars;
  for (uint32_t predLabel : cfg()->preds(label)) {
    for (auto var_val : label2ssa_map_[predLabel]) {
      uint32_t varId = var_val.first;
      liveVars[varId] = var_val.second;
    }
  }
  // Add all stored variables in loop. Set their default value id to zero.
  for (auto bi = block_itr; (*bi)->id() != mergeLabel; ++bi) {
    ir::BasicBlock* bp = *bi;
    for (auto ii = bp->begin(); ii != bp->end(); ++ii) {
      if (ii->opcode() != SpvOpStore) {
        continue;
      }
      uint32_t varId;
      (void)GetPtr(&*ii, &varId);
      if (!IsTargetVar(varId)) {
        continue;
      }
      liveVars[varId] = 0;
    }
  }
  // Insert phi for all live variables that require them. All variables
  // defined in loop require a phi. Otherwise all variables
  // with differing predecessor values require a phi.
  auto insertItr = (*block_itr)->begin();
  for (auto var_val : liveVars) {
    const uint32_t varId = var_val.first;
    if (!IsLiveAfter(varId, label)) {
      continue;
    }
    const uint32_t val0Id = var_val.second;
    bool needsPhi = false;
    if (val0Id != 0) {
      for (uint32_t predLabel : cfg()->preds(label)) {
        // Skip back edge predecessor.
        if (predLabel == backLabel) continue;
        const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
        // Missing (undef) values always cause difference with (defined) value
        if (var_val_itr == label2ssa_map_[predLabel].end()) {
          needsPhi = true;
          break;
        }
        if (var_val_itr->second != val0Id) {
          needsPhi = true;
          break;
        }
      }
    } else {
      needsPhi = true;
    }

    // If val is the same for all predecessors, enter it in map
    if (!needsPhi) {
      label2ssa_map_[label].insert(var_val);
      continue;
    }

    // Val differs across predecessors. Add phi op to block and
    // add its result id to the map. For back edge predecessor,
    // use the variable id. We will patch this after visiting back
    // edge predecessor. For predecessors that do not define a value,
    // use undef.
    std::vector<ir::Operand> phi_in_operands;
    uint32_t typeId = GetPointeeTypeId(get_def_use_mgr()->GetDef(varId));
    for (uint32_t predLabel : cfg()->preds(label)) {
      uint32_t valId;
      if (predLabel == backLabel) {
        valId = varId;
      } else {
        const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
        if (var_val_itr == label2ssa_map_[predLabel].end())
          valId = Type2Undef(typeId);
        else
          valId = var_val_itr->second;
      }
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {valId}});
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {predLabel}});
    }
    const uint32_t phiId = TakeNextId();
    std::unique_ptr<ir::Instruction> newPhi(new ir::Instruction(
        context(), SpvOpPhi, typeId, phiId, phi_in_operands));
    // The only phis requiring patching are the ones we create.
    phis_to_patch_.insert(phiId);
    // Only analyze the phi define now; analyze the phi uses after the
    // phi backedge predecessor value is patched.
    get_def_use_mgr()->AnalyzeInstDef(&*newPhi);
    insertItr = insertItr.InsertBefore(std::move(newPhi));
    ++insertItr;
    label2ssa_map_[label].insert({varId, phiId});
  }
}

void MemPass::SSABlockInitMultiPred(ir::BasicBlock* block_ptr) {
  const uint32_t label = block_ptr->id();
  // Collect all live variables and a default value for each across all
  // predecesors. Must be ordered map because phis are generated based on
  // order and test results will otherwise vary across platforms.
  std::map<uint32_t, uint32_t> liveVars;
  for (uint32_t predLabel : cfg()->preds(label)) {
    assert(visitedBlocks_.find(predLabel) != visitedBlocks_.end());
    for (auto var_val : label2ssa_map_[predLabel]) {
      const uint32_t varId = var_val.first;
      liveVars[varId] = var_val.second;
    }
  }
  // For each live variable, look for a difference in values across
  // predecessors that would require a phi and insert one.
  auto insertItr = block_ptr->begin();
  for (auto var_val : liveVars) {
    const uint32_t varId = var_val.first;
    if (!IsLiveAfter(varId, label)) continue;
    const uint32_t val0Id = var_val.second;
    bool differs = false;
    for (uint32_t predLabel : cfg()->preds(label)) {
      const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
      // Missing values cause a difference because we'll need to create an
      // undef for that predecessor.
      if (var_val_itr == label2ssa_map_[predLabel].end()) {
        differs = true;
        break;
      }
      if (var_val_itr->second != val0Id) {
        differs = true;
        break;
      }
    }
    // If val is the same for all predecessors, enter it in map
    if (!differs) {
      label2ssa_map_[label].insert(var_val);
      continue;
    }
    // Val differs across predecessors. Add phi op to block and add its result
    // id to the map.
    std::vector<ir::Operand> phi_in_operands;
    const uint32_t typeId = GetPointeeTypeId(get_def_use_mgr()->GetDef(varId));
    for (uint32_t predLabel : cfg()->preds(label)) {
      const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
      // If variable not defined on this path, use undef
      const uint32_t valId = (var_val_itr != label2ssa_map_[predLabel].end())
                                 ? var_val_itr->second
                                 : Type2Undef(typeId);
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {valId}});
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {predLabel}});
    }
    const uint32_t phiId = TakeNextId();
    std::unique_ptr<ir::Instruction> newPhi(new ir::Instruction(
        context(), SpvOpPhi, typeId, phiId, phi_in_operands));
    get_def_use_mgr()->AnalyzeInstDefUse(&*newPhi);
    insertItr = insertItr.InsertBefore(std::move(newPhi));
    ++insertItr;
    label2ssa_map_[label].insert({varId, phiId});
  }
}

void MemPass::SSABlockInit(std::list<ir::BasicBlock*>::iterator block_itr) {
  const size_t numPreds = cfg()->preds((*block_itr)->id()).size();
  if (numPreds == 0) return;
  if (numPreds == 1)
    SSABlockInitSinglePred(*block_itr);
  else if ((*block_itr)->IsLoopHeader())
    SSABlockInitLoopHeader(block_itr);
  else
    SSABlockInitMultiPred(*block_itr);
}

bool MemPass::IsTargetVar(uint32_t varId) {
  if (seen_non_target_vars_.find(varId) != seen_non_target_vars_.end())
    return false;
  if (seen_target_vars_.find(varId) != seen_target_vars_.end()) return true;
  const ir::Instruction* varInst = get_def_use_mgr()->GetDef(varId);
  if (varInst->opcode() != SpvOpVariable) return false;
  ;
  const uint32_t varTypeId = varInst->type_id();
  const ir::Instruction* varTypeInst = get_def_use_mgr()->GetDef(varTypeId);
  if (varTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx) !=
      SpvStorageClassFunction) {
    seen_non_target_vars_.insert(varId);
    return false;
  }
  const uint32_t varPteTypeId =
      varTypeInst->GetSingleWordInOperand(kTypePointerTypeIdInIdx);
  ir::Instruction* varPteTypeInst = get_def_use_mgr()->GetDef(varPteTypeId);
  if (!IsTargetType(varPteTypeInst)) {
    seen_non_target_vars_.insert(varId);
    return false;
  }
  seen_target_vars_.insert(varId);
  return true;
}

void MemPass::PatchPhis(uint32_t header_id, uint32_t back_id) {
  ir::BasicBlock* header = cfg()->block(header_id);
  auto phiItr = header->begin();
  for (; phiItr->opcode() == SpvOpPhi; ++phiItr) {
    // Only patch phis that we created in a loop header.
    // There might be other phis unrelated to our optimizations.
    if (0 == phis_to_patch_.count(phiItr->result_id())) continue;

    // Find phi operand index for back edge
    uint32_t cnt = 0;
    uint32_t idx = phiItr->NumInOperands();
    phiItr->ForEachInId([&cnt, &back_id, &idx](uint32_t* iid) {
      if (cnt % 2 == 1 && *iid == back_id) idx = cnt - 1;
      ++cnt;
    });
    assert(idx != phiItr->NumInOperands());
    // Replace temporary phi operand with variable's value in backedge block
    // map. Use undef if variable not in map.
    const uint32_t varId = phiItr->GetSingleWordInOperand(idx);
    const auto valItr = label2ssa_map_[back_id].find(varId);
    uint32_t valId =
        (valItr != label2ssa_map_[back_id].end())
            ? valItr->second
            : Type2Undef(GetPointeeTypeId(get_def_use_mgr()->GetDef(varId)));
    phiItr->SetInOperand(idx, {valId});
    // Analyze uses now that they are complete
    get_def_use_mgr()->AnalyzeInstUse(&*phiItr);
  }
}

Pass::Status MemPass::InsertPhiInstructions(ir::Function* func) {
  // TODO(dnovillo) the current Phi placement mechanism assumes structured
  // control-flow. This should be generalized
  // (https://github.com/KhronosGroup/SPIRV-Tools/issues/893).
  assert(get_module()->HasCapability(SpvCapabilityShader) &&
         "This only works on structured control flow");

  // Initialize the data structures used to insert Phi instructions.
  InitSSARewrite(func);

  // Process all blocks in structured order. This is just one way (the
  // simplest?) to make sure all predecessors blocks are processed before
  // a block itself.
  std::list<ir::BasicBlock*> structuredOrder;
  cfg()->ComputeStructuredOrder(func, cfg()->pseudo_entry_block(),
                                &structuredOrder);
  for (auto bi = structuredOrder.begin(); bi != structuredOrder.end(); ++bi) {
    // Skip pseudo entry block
    if (cfg()->IsPseudoEntryBlock(*bi)) {
      continue;
    }

    // Initialize this block's label2ssa_map_ entry using predecessor maps.
    // Then process all stores and loads of targeted variables.
    SSABlockInit(bi);
    ir::BasicBlock* bp = *bi;
    const uint32_t label = bp->id();
    for (auto ii = bp->begin(); ii != bp->end(); ++ii) {
      switch (ii->opcode()) {
        case SpvOpStore: {
          uint32_t varId;
          (void)GetPtr(&*ii, &varId);
          if (!IsTargetVar(varId)) break;
          // Register new stored value for the variable
          label2ssa_map_[label][varId] =
              ii->GetSingleWordInOperand(kStoreValIdInIdx);
        } break;
        case SpvOpLoad: {
          uint32_t varId;
          (void)GetPtr(&*ii, &varId);
          if (!IsTargetVar(varId)) break;
          uint32_t replId = 0;
          const auto ssaItr = label2ssa_map_.find(label);
          if (ssaItr != label2ssa_map_.end()) {
            const auto valItr = ssaItr->second.find(varId);
            if (valItr != ssaItr->second.end()) replId = valItr->second;
          }
          // If variable is not defined, use undef
          if (replId == 0) {
            replId =
                Type2Undef(GetPointeeTypeId(get_def_use_mgr()->GetDef(varId)));
          }
          // Replace load's id with the last stored value id for variable
          // and delete load. Kill any names or decorates using id before
          // replacing to prevent incorrect replacement in those instructions.
          const uint32_t loadId = ii->result_id();
          context()->KillNamesAndDecorates(loadId);
          (void)context()->ReplaceAllUsesWith(loadId, replId);
          context()->KillInst(&*ii);
        } break;
        default: { } break; }
    }
    visitedBlocks_.insert(label);
    // Look for successor backedge and patch phis in loop header
    // if found.
    uint32_t header = 0;
    bp->ForEachSuccessorLabel([&header, this](uint32_t succ) {
      if (visitedBlocks_.find(succ) == visitedBlocks_.end()) return;
      assert(header == 0);
      header = succ;
    });
    if (header != 0) PatchPhis(header, label);
  }

  return Status::SuccessWithChange;
}

// Remove all |phi| operands coming from unreachable blocks (i.e., blocks not in
// |reachable_blocks|).  There are two types of removal that this function can
// perform:
//
// 1- Any operand that comes directly from an unreachable block is completely
//    removed.  Since the block is unreachable, the edge between the unreachable
//    block and the block holding |phi| has been removed.
//
// 2- Any operand that comes via a live block and was defined at an unreachable
//    block gets its value replaced with an OpUndef value. Since the argument
//    was generated in an unreachable block, it no longer exists, so it cannot
//    be referenced.  However, since the value does not reach |phi| directly
//    from the unreachable block, the operand cannot be removed from |phi|.
//    Therefore, we replace the argument value with OpUndef.
//
// For example, in the switch() below, assume that we want to remove the
// argument with value %11 coming from block %41.
//
//          [ ... ]
//          %41 = OpLabel                    <--- Unreachable block
//          %11 = OpLoad %int %y
//          [ ... ]
//                OpSelectionMerge %16 None
//                OpSwitch %12 %16 10 %13 13 %14 18 %15
//          %13 = OpLabel
//                OpBranch %16
//          %14 = OpLabel
//                OpStore %outparm %int_14
//                OpBranch %16
//          %15 = OpLabel
//                OpStore %outparm %int_15
//                OpBranch %16
//          %16 = OpLabel
//          %30 = OpPhi %int %11 %41 %int_42 %13 %11 %14 %11 %15
//
// Since %41 is now an unreachable block, the first operand of |phi| needs to
// be removed completely.  But the operands (%11 %14) and (%11 %15) cannot be
// removed because %14 and %15 are reachable blocks.  Since %11 no longer exist,
// in those arguments, we replace all references to %11 with an OpUndef value.
// This results in |phi| looking like:
//
//           %50 = OpUndef %int
//           [ ... ]
//           %30 = OpPhi %int %int_42 %13 %50 %14 %50 %15
void MemPass::RemovePhiOperands(
    ir::Instruction* phi,
    std::unordered_set<ir::BasicBlock*> reachable_blocks) {
  std::vector<ir::Operand> keep_operands;
  uint32_t type_id = 0;
  // The id of an undefined value we've generated.
  uint32_t undef_id = 0;

  // Traverse all the operands in |phi|. Build the new operand vector by adding
  // all the original operands from |phi| except the unwanted ones.
  for (uint32_t i = 0; i < phi->NumOperands();) {
    if (i < 2) {
      // The first two arguments are always preserved.
      keep_operands.push_back(phi->GetOperand(i));
      ++i;
      continue;
    }

    // The remaining Phi arguments come in pairs. Index 'i' contains the
    // variable id, index 'i + 1' is the originating block id.
    assert(i % 2 == 0 && i < phi->NumOperands() - 1 &&
           "malformed Phi arguments");

    ir::BasicBlock* in_block = cfg()->block(phi->GetSingleWordOperand(i + 1));
    if (reachable_blocks.find(in_block) == reachable_blocks.end()) {
      // If the incoming block is unreachable, remove both operands as this
      // means that the |phi| has lost an incoming edge.
      i += 2;
      continue;
    }

    // In all other cases, the operand must be kept but may need to be changed.
    uint32_t arg_id = phi->GetSingleWordOperand(i);
    ir::Instruction* arg_def_instr = get_def_use_mgr()->GetDef(arg_id);
    ir::BasicBlock* def_block = context()->get_instr_block(arg_def_instr);
    if (def_block &&
        reachable_blocks.find(def_block) == reachable_blocks.end()) {
      // If the current |phi| argument was defined in an unreachable block, it
      // means that this |phi| argument is no longer defined. Replace it with
      // |undef_id|.
      if (!undef_id) {
        type_id = arg_def_instr->type_id();
        undef_id = Type2Undef(type_id);
      }
      keep_operands.push_back(
          ir::Operand(spv_operand_type_t::SPV_OPERAND_TYPE_ID, {undef_id}));
    } else {
      // Otherwise, the argument comes from a reachable block or from no block
      // at all (meaning that it was defined in the global section of the
      // program).  In both cases, keep the argument intact.
      keep_operands.push_back(phi->GetOperand(i));
    }

    keep_operands.push_back(phi->GetOperand(i + 1));

    i += 2;
  }

  context()->ForgetUses(phi);
  phi->ReplaceOperands(keep_operands);
  context()->AnalyzeUses(phi);
}

void MemPass::RemoveBlock(ir::Function::iterator* bi) {
  auto& rm_block = **bi;

  // Remove instructions from the block.
  rm_block.ForEachInst([&rm_block, this](ir::Instruction* inst) {
    // Note that we do not kill the block label instruction here. The label
    // instruction is needed to identify the block, which is needed by the
    // removal of phi operands.
    if (inst != rm_block.GetLabelInst()) {
      context()->KillInst(inst);
    }
  });

  // Remove the label instruction last.
  auto label = rm_block.GetLabelInst();
  context()->KillInst(label);

  *bi = bi->Erase();
}

bool MemPass::RemoveUnreachableBlocks(ir::Function* func) {
  bool modified = false;

  // Mark reachable all blocks reachable from the function's entry block.
  std::unordered_set<ir::BasicBlock*> reachable_blocks;
  std::unordered_set<ir::BasicBlock*> visited_blocks;
  std::queue<ir::BasicBlock*> worklist;
  reachable_blocks.insert(func->entry().get());

  // Initially mark the function entry point as reachable.
  worklist.push(func->entry().get());

  auto mark_reachable = [&reachable_blocks, &visited_blocks, &worklist,
                         this](uint32_t label_id) {
    auto successor = cfg()->block(label_id);
    if (visited_blocks.count(successor) == 0) {
      reachable_blocks.insert(successor);
      worklist.push(successor);
      visited_blocks.insert(successor);
    }
  };

  // Transitively mark all blocks reachable from the entry as reachable.
  while (!worklist.empty()) {
    ir::BasicBlock* block = worklist.front();
    worklist.pop();

    // All the successors of a live block are also live.
    block->ForEachSuccessorLabel(mark_reachable);

    // All the Merge and ContinueTarget blocks of a live block are also live.
    block->ForMergeAndContinueLabel(mark_reachable);
  }

  // Update operands of Phi nodes that reference unreachable blocks.
  for (auto& block : *func) {
    // If the block is about to be removed, don't bother updating its
    // Phi instructions.
    if (reachable_blocks.count(&block) == 0) {
      continue;
    }

    // If the block is reachable and has Phi instructions, remove all
    // operands from its Phi instructions that reference unreachable blocks.
    // If the block has no Phi instructions, this is a no-op.
    block.ForEachPhiInst([&reachable_blocks, this](ir::Instruction* phi) {
      RemovePhiOperands(phi, reachable_blocks);
    });
  }

  // Erase unreachable blocks.
  for (auto ebi = func->begin(); ebi != func->end();) {
    if (reachable_blocks.count(&*ebi) == 0) {
      RemoveBlock(&ebi);
      modified = true;
    } else {
      ++ebi;
    }
  }

  return modified;
}

bool MemPass::CFGCleanup(ir::Function* func) {
  bool modified = false;
  modified |= RemoveUnreachableBlocks(func);
  return modified;
}

}  // namespace opt
}  // namespace spvtools
