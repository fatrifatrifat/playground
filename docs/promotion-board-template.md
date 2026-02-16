# Strategy Promotion Board

## Strategy Card

- **Strategy ID:**  
- **Version:**  
- **Owner (Research):**  
- **Owner (Dev):**  
- **Asset Class:** (Equity / Crypto / Bond / Multi-asset)  
- **Universe:**  
- **Venue(s):**  
- **Style:** (MM, stat-arb, trend, carry, etc.)  
- **Primary Horizon:** (intraday / multi-day / etc.)  
- **Created On:**  
- **Last Updated:**  

## Research Summary

- **Hypothesis (1-3 lines):**  
- **Key alpha features:**  
- **Data sources used:**  
- **Known weaknesses / regimes where it fails:**  
- **Expected capacity constraints:**  

## Promotion Ladder Status

- [ ] Stage 0 — Research Prototype Complete  
- [ ] Stage 1 — Contract Compliance (protobuf + schema checks)  
- [ ] Stage 2 — Historical Replay Pass  
- [ ] Stage 3 — Paper Trading Shadow Pass  
- [ ] Stage 4 — Live Canary Approval  
- [ ] Stage 5 — Scaled Live Approval  

---

## Gate 1: Contract Compliance

**Checklist**
- [ ] Emits valid `StrategySignal` (schema vX.Y)
- [ ] Includes `strategy_id`, `strategy_version`, `signal_id`
- [ ] Uses canonical `instrument_id`
- [ ] TTL, side, qty, and price constraints validated
- [ ] Traceability tags attached (`research_run_id`, `correlation_id`)

**Result**
- **Status:** PASS / FAIL  
- **Reviewer:**  
- **Date:**  
- **Notes:**  

---

## Gate 2: Historical Replay

**Window**
- **Train period:**  
- **Validation period:**  
- **Out-of-sample period:**  

**Metrics (must define thresholds before run)**
- **Net PnL:**  
- **Sharpe / Sortino:**  
- **Max drawdown:**  
- **Turnover:**  
- **Hit rate:**  
- **Avg slippage (bps):**  
- **Tail event behavior:**  

**Determinism**
- [ ] Same inputs => same outputs across repeated runs
- [ ] Replay artifacts archived

**Result**
- **Status:** PASS / FAIL  
- **Reviewer(s):**  
- **Date:**  
- **Notes:**  

---

## Gate 3: Paper Trading Shadow

**Run setup**
- **Start date/time:**  
- **End date/time:**  
- **Market hours coverage:**  
- **Latency budget target (p99):**  

**Operational checks**
- [ ] No schema rejects in gateway
- [ ] No OMS state violations
- [ ] Risk rejects are explainable and expected
- [ ] No dropped market data events
- [ ] Monitoring dashboard complete

**Paper metrics**
- **Paper PnL:**  
- **Order reject rate:**  
- **Fill ratio:**  
- **Decision-to-order latency p50/p95/p99:**  
- **Order-to-ack latency p50/p95/p99:**  

**Result**
- **Status:** PASS / FAIL  
- **Reviewer(s):**  
- **Date:**  
- **Notes:**  

---

## Gate 4: Live Canary

**Hard limits**
- **Max order qty:**  
- **Max position:**  
- **Max daily loss:**  
- **Max notional:**  
- **Kill-switch condition(s):**  

**Canary plan**
- **Capital/notional allocation:**  
- **Duration:**  
- **Trading windows:**  
- **Rollback owner:**  

**Result**
- **Status:** PASS / FAIL  
- **Reviewer(s):**  
- **Date:**  
- **Notes:**  

---

## Gate 5: Scale-Up

**Scale plan**
- **Current allocation:**  
- **Next allocation:**  
- **Conditions to increase:**  
- **Conditions to freeze/reduce:**  

**Result**
- **Status:** PASS / FAIL  
- **Reviewer(s):**  
- **Date:**  
- **Notes:**  

---

## Risk Sign-off Matrix

- **Research Lead:** APPROVE / REJECT  
- **Dev Lead:** APPROVE / REJECT  
- **Risk Owner:** APPROVE / REJECT  
- **Release Manager:** APPROVE / REJECT  

**Final Decision:** PROMOTE / HOLD / ROLLBACK  
**Decision Date:**  
**Reasoning:**  

---

## Incident / Rollback Plan

- **Kill switch command path documented:** [ ] Yes [ ] No  
- **Fallback strategy:**  
- **Open orders cancel procedure:**  
- **State reconciliation procedure:**  
- **Post-incident review owner:**  

---

## Change Log

| Date | Version | Change | Author | Review Link |
|---|---|---|---|---|
| | | | | |
| | | | | |
