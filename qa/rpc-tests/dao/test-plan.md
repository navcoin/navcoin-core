# NavCoin DAO Test Plan

This file is a map of the tests scenarios which will be covered to ensure the CFund is fully tested before release. This is currently a work in progress, so if you notice there are missing scenarios please add them to this file following the same format/template. If you want to author a test based on a scenario here then please go ahead and claim it. Just make sure you claim and check off any scenarios which you write so others are aware of what is still todo.

## Scenarios

### Scenario 001

| ID          |  CFUND_001 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Ready |
| Description | It should create a proposal and let it expire |
| File        | `./001-proposal-expires.py`

1. **Given** I have created a proposal
2. **When** The proposal has received no votes
3. **Then** the proposal should expire 

#### Test Steps
- Activate CFund
- Donate to the CFund
- Create a proposal
- End the voting cycles with no votes
- Check proposal expired
