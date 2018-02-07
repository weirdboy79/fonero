// Copyright (c) 2017-2018, The Fonero Project.
// Copyright (c) 2014-2017 The Monero Project.
// Portions Copyright (c) 2012-2013 The Cryptonote developers.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "include_base_utils.h"
using namespace epee;

#include "cryptonote_tx_utils.h"
#include "cryptonote_config.h"
#include "cryptonote_basic/miner.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "ringct/rctSigs.h"

namespace cryptonote
{
  //---------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, uint64_t already_generated_coins, size_t current_block_size, uint64_t fee, const account_public_address &miner_address, transaction& tx, const blobdata& extra_nonce, uint8_t hard_fork_version) {
    tx.vin.clear();
    tx.vout.clear();
    tx.extra.clear();

    keypair txkey = keypair::generate();
    add_tx_pub_key_to_extra(tx, txkey.pub);
    if(!extra_nonce.empty())
      if(!add_extra_nonce_to_tx_extra(tx.extra, extra_nonce))
        return false;

    txin_gen in;
    in.height = height;

    uint64_t block_reward;
    if(!get_block_reward(median_size, current_block_size, already_generated_coins, block_reward, hard_fork_version, height))
    {
      LOG_PRINT_L0("Block is too big");
      return false;
    }

#if defined(DEBUG_CREATE_BLOCK_TEMPLATE)
    LOG_PRINT_L1("Creating block template: reward " << block_reward <<
      ", fee " << fee);
#endif
    block_reward += fee;

    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);;
    crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
    bool r = crypto::generate_key_derivation(miner_address.m_view_public_key, txkey.sec, derivation);
    CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to generate_key_derivation(" << miner_address.m_view_public_key << ", " << txkey.sec << ")");

    r = crypto::derive_public_key(derivation, 0, miner_address.m_spend_public_key, out_eph_public_key);
    CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to derive_public_key(" << derivation << ", " << "0" << ", "<< miner_address.m_spend_public_key << ")");

    txout_to_key tk;
    tk.key = out_eph_public_key;

    tx_out out;
    out.amount = block_reward;
    out.target = tk;
    tx.vout.push_back(out);

    tx.version = CURRENT_TRANSACTION_VERSION;

    //lock
    tx.unlock_time = height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
    tx.vin.push_back(in);

    tx.invalidate_hashes();

    //LOG_PRINT("MINER_TX generated ok, block_reward=" << print_money(block_reward) << "("  << print_money(block_reward - fee) << "+" << print_money(fee)
    //  << "), current_block_size=" << current_block_size << ", already_generated_coins=" << already_generated_coins << ", tx_id=" << get_transaction_hash(tx), LOG_LEVEL_2);
    return true;
  }
  //---------------------------------------------------------------
  crypto::public_key get_destination_view_key_pub(const std::vector<tx_destination_entry> &destinations, const account_keys &sender_keys)
  {
    if (destinations.empty())
      return null_pkey;
    for (size_t n = 1; n < destinations.size(); ++n)
    {
      if (!memcmp(&destinations[n].addr, &sender_keys.m_account_address, sizeof(destinations[0].addr)))
        continue;
      if (destinations[n].amount == 0)
        continue;
      if (memcmp(&destinations[n].addr, &destinations[0].addr, sizeof(destinations[0].addr)))
        return null_pkey;
    }
    return destinations[0].addr.m_view_public_key;
  }
  //---------------------------------------------------------------
  bool construct_tx_and_get_tx_key(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, const std::vector<tx_destination_entry>& destinations, std::vector<uint8_t> extra, transaction& tx, uint64_t unlock_time, crypto::secret_key &tx_key)
  {
    std::vector<rct::key> amount_keys;
    tx.set_null();
    amount_keys.clear();

    tx.version = CURRENT_TRANSACTION_VERSION;
    tx.unlock_time = unlock_time;

    tx.extra = extra;
    keypair txkey = keypair::generate();
    remove_field_from_tx_extra(tx.extra, typeid(tx_extra_pub_key));
    add_tx_pub_key_to_extra(tx, txkey.pub);
    tx_key = txkey.sec;

    // if we have a stealth payment id, find it and encrypt it with the tx key now
    std::vector<tx_extra_field> tx_extra_fields;
    if (parse_tx_extra(tx.extra, tx_extra_fields))
    {
      tx_extra_nonce extra_nonce;
      if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
      {
        crypto::hash8 payment_id = null_hash8;
        if (get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
        {
          LOG_PRINT_L2("Encrypting payment id " << payment_id);
          crypto::public_key view_key_pub = get_destination_view_key_pub(destinations, sender_account_keys);
          if (view_key_pub == null_pkey)
          {
            LOG_ERROR("Destinations have to have exactly one output to support encrypted payment ids");
            return false;
          }

          if (!encrypt_payment_id(payment_id, view_key_pub, txkey.sec))
          {
            LOG_ERROR("Failed to encrypt payment id");
            return false;
          }

          std::string extra_nonce;
          set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
          remove_field_from_tx_extra(tx.extra, typeid(tx_extra_nonce));
          if (!add_extra_nonce_to_tx_extra(tx.extra, extra_nonce))
          {
            LOG_ERROR("Failed to add encrypted payment id to tx extra");
            return false;
          }
          LOG_PRINT_L1("Encrypted payment ID: " << payment_id);
        }
      }
    }
    else
    {
      LOG_ERROR("Failed to parse tx extra");
      return false;
    }

    struct input_generation_context_data
    {
      keypair in_ephemeral;
    };
    std::vector<input_generation_context_data> in_contexts;

    uint64_t summary_inputs_money = 0;
    //fill inputs
    int idx = -1;
    for(const tx_source_entry& src_entr:  sources)
    {
      ++idx;
      if(src_entr.real_output >= src_entr.outputs.size())
      {
        LOG_ERROR("real_output index (" << src_entr.real_output << ")bigger than output_keys.size()=" << src_entr.outputs.size());
        return false;
      }
      summary_inputs_money += src_entr.amount;

      //key_derivation recv_derivation;
      in_contexts.push_back(input_generation_context_data());
      keypair& in_ephemeral = in_contexts.back().in_ephemeral;
      crypto::key_image img;
      if(!generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_ephemeral, img))
        return false;

      //check that derivated key is equal with real output key
      if( !(in_ephemeral.pub == src_entr.outputs[src_entr.real_output].second.dest) )
      {
        LOG_ERROR("derived public key mismatch with output public key at index " << idx << ", real out " << src_entr.real_output << "! "<< ENDL << "derived_key:"
          << string_tools::pod_to_hex(in_ephemeral.pub) << ENDL << "real output_public_key:"
          << string_tools::pod_to_hex(src_entr.outputs[src_entr.real_output].second) );
        LOG_ERROR("amount " << src_entr.amount);
        LOG_ERROR("tx pubkey " << src_entr.real_out_tx_key << ", real_output_in_tx_index " << src_entr.real_output_in_tx_index);
        return false;
      }

      //put key image into tx input
      txin_to_key input_to_key;
      input_to_key.amount = src_entr.amount;
      input_to_key.k_image = img;

      //fill outputs array and use relative offsets
      for(const tx_source_entry::output_entry& out_entry: src_entr.outputs)
        input_to_key.key_offsets.push_back(out_entry.first);

      input_to_key.key_offsets = absolute_output_offsets_to_relative(input_to_key.key_offsets);
      tx.vin.push_back(input_to_key);
    }

    // "Shuffle" outs
    std::vector<tx_destination_entry> shuffled_dsts(destinations);
    std::random_shuffle(shuffled_dsts.begin(), shuffled_dsts.end(), [](unsigned int i) { return crypto::rand<unsigned int>() % i; });

    uint64_t summary_outs_money = 0;
    //fill outputs
    size_t output_index = 0;
    for(const tx_destination_entry& dst_entr:  shuffled_dsts)
    {
      crypto::key_derivation derivation;
      crypto::public_key out_eph_public_key;
      bool r = crypto::generate_key_derivation(dst_entr.addr.m_view_public_key, txkey.sec, derivation);
      CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to generate_key_derivation(" << dst_entr.addr.m_view_public_key << ", " << txkey.sec << ")");

      crypto::secret_key scalar1;
      crypto::derivation_to_scalar(derivation, output_index, scalar1);
      amount_keys.push_back(rct::sk2rct(scalar1));
      r = crypto::derive_public_key(derivation, output_index, dst_entr.addr.m_spend_public_key, out_eph_public_key);
      CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to derive_public_key(" << derivation << ", " << output_index << ", "<< dst_entr.addr.m_spend_public_key << ")");

      tx_out out;
      out.amount = dst_entr.amount;
      txout_to_key tk;
      tk.key = out_eph_public_key;
      out.target = tk;
      tx.vout.push_back(out);
      output_index++;
      summary_outs_money += dst_entr.amount;
    }

    //check money
    if(summary_outs_money > summary_inputs_money )
    {
      LOG_ERROR("Transaction inputs money ("<< summary_inputs_money << ") less than outputs money (" << summary_outs_money << ")");
      return false;
    }

    // check for watch only wallet
    bool zero_secret_key = true;
    for (size_t i = 0; i < sizeof(sender_account_keys.m_spend_secret_key); ++i)
      zero_secret_key &= (sender_account_keys.m_spend_secret_key.data[i] == 0);
    if (zero_secret_key)
    {
      MDEBUG("Null secret key, skipping signatures");
    }

    size_t n_total_outs = sources[0].outputs.size(); // only for non-simple rct

    // the non-simple version is slightly smaller, but assumes all real inputs
    // are on the same index, so can only be used if there just one ring.
    bool use_simple_rct = sources.size() > 1;

    if (!use_simple_rct)
    {
      // non simple ringct requires all real inputs to be at the same index for all inputs
      for(const tx_source_entry& src_entr:  sources)
      {
        if(src_entr.real_output != sources.begin()->real_output)
        {
          LOG_ERROR("All inputs must have the same index for non-simple ringct");
          return false;
        }
      }

      // enforce same mixin for all outputs
      for (size_t i = 1; i < sources.size(); ++i) {
        if (n_total_outs != sources[i].outputs.size()) {
          LOG_ERROR("Non-simple ringct transaction has varying mixin");
          return false;
        }
      }
    }

    uint64_t amount_in = 0, amount_out = 0;
    rct::ctkeyV inSk;
    // mixRing indexing is done the other way round for simple
    rct::ctkeyM mixRing(use_simple_rct ? sources.size() : n_total_outs);
    rct::keyV dests;
    std::vector<uint64_t> inamounts, outamounts;
    std::vector<unsigned int> index;
    for (size_t i = 0; i < sources.size(); ++i)
    {
      rct::ctkey ctkey;
      amount_in += sources[i].amount;
      inamounts.push_back(sources[i].amount);
      index.push_back(sources[i].real_output);
      // inSk: (secret key, mask)
      ctkey.dest = rct::sk2rct(in_contexts[i].in_ephemeral.sec);
      ctkey.mask = sources[i].mask;
      inSk.push_back(ctkey);
      // inPk: (public key, commitment)
      // will be done when filling in mixRing
    }
    for (size_t i = 0; i < tx.vout.size(); ++i)
    {
      dests.push_back(rct::pk2rct(boost::get<txout_to_key>(tx.vout[i].target).key));
      outamounts.push_back(tx.vout[i].amount);
      amount_out += tx.vout[i].amount;
    }

    if (use_simple_rct)
    {
      // mixRing indexing is done the other way round for simple
      for (size_t i = 0; i < sources.size(); ++i)
      {
        mixRing[i].resize(sources[i].outputs.size());
        for (size_t n = 0; n < sources[i].outputs.size(); ++n)
        {
          mixRing[i][n] = sources[i].outputs[n].second;
        }
      }
    }
    else
    {
      for (size_t i = 0; i < n_total_outs; ++i) // same index assumption
      {
        mixRing[i].resize(sources.size());
        for (size_t n = 0; n < sources.size(); ++n)
        {
          mixRing[i][n] = sources[n].outputs[i].second;
        }
      }
    }

    // fee
    if (!use_simple_rct && amount_in > amount_out)
      outamounts.push_back(amount_in - amount_out);

    // zero out all amounts to mask rct outputs, real amounts are now encrypted
    for (size_t i = 0; i < tx.vin.size(); ++i)
    {
      boost::get<txin_to_key>(tx.vin[i]).amount = 0;
    }
    for (size_t i = 0; i < tx.vout.size(); ++i)
      tx.vout[i].amount = 0;

    crypto::hash tx_prefix_hash;
    get_transaction_prefix_hash(tx, tx_prefix_hash);
    rct::ctkeyV outSk;
    if (use_simple_rct)
      tx.rct_signatures = rct::genRctSimple(rct::hash2rct(tx_prefix_hash), inSk, dests, inamounts, outamounts, amount_in - amount_out, mixRing, amount_keys, index, outSk);
    else
      tx.rct_signatures = rct::genRct(rct::hash2rct(tx_prefix_hash), inSk, dests, outamounts, mixRing, amount_keys, sources[0].real_output, outSk); // same index assumption

    CHECK_AND_ASSERT_MES(tx.vout.size() == outSk.size(), false, "outSk size does not match vout");

    MCINFO("construct_tx", "transaction_created: " << get_transaction_hash(tx) << ENDL << obj_to_json_str(tx) << ENDL);

    tx.invalidate_hashes();

    return true;
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources, const std::vector<tx_destination_entry>& destinations, std::vector<uint8_t> extra, transaction& tx, uint64_t unlock_time)
  {
     crypto::secret_key tx_key;
     return construct_tx_and_get_tx_key(sender_account_keys, sources, destinations, extra, tx, unlock_time, tx_key);
  }
  //---------------------------------------------------------------
  bool generate_genesis_block(block& bl)
  {
    //genesis block
    bl = boost::value_initialized<block>();

    account_public_address ac = boost::value_initialized<account_public_address>();
    std::vector<size_t> sz;
    construct_miner_tx(0, 0, 0, 0, 0, ac, bl.miner_tx); // zero fee in genesis
    blobdata txb = tx_to_blob(bl.miner_tx);
    std::string hex_tx_represent = string_tools::buff_to_hex_nodelimer(txb);
    blobdata tx_bl;
    string_tools::parse_hexstr_to_binbuff(config::GENESIS_TX, tx_bl);
    bool r = parse_and_validate_tx_from_blob(tx_bl, bl.miner_tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse coinbase tx from hard coded blob");
    bl.major_version = CURRENT_BLOCK_MAJOR_VERSION;
    bl.minor_version = CURRENT_BLOCK_MINOR_VERSION;
    bl.timestamp = 0;
    bl.nonce = config::GENESIS_NONCE;
    miner::find_nonce_for_given_block(bl, 1, 0);
    bl.invalidate_hashes();
    return true;
  }
  //---------------------------------------------------------------
}
