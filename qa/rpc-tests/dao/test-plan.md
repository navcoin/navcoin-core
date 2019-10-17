# NavCoin DAO Test Plan

This file is a map of the tests scenarios which will be covered to ensure the DAO is fully tested before release. This is currently a work in progress, so if you notice there are missing scenarios please add them to this file following the same format/template. If you want to create a test based on a scenario here then please go ahead and claim it. Just make sure you claim and check off any scenarios which you write so others are aware of what is still todo.

## Scenarios

### Scenario 001

| ID          |  DAO_001 |
| ----------- | -----: |
| Reporter    | proletesseract |
| Author      | - |
| Satus       | Ready |
| Description | It should pass a consultation with included answers |
| File        | `./dao-001-pass-with-included.py`

1. **Given** I have created a consultation with included answers
2. **When** I support the consultation and vote for an answer
3. **Then** the consultation should pass

#### Test Steps
- Activate DAO
- Create consultation with answers
- Support consultation
- Vote for answer
- Check consultation finished with correct answer

### Scenario 002

| ID          |  DAO_002 |
| ----------- | -----: |
| Reporter    | proletesseract |
| Author      | - |
| Satus       | Ready |
| Description | It should pass a consultation with proposed answers |
| File        | `./dao-002-pass-with-proposed.py`

1. **Given** I have created a consultation without answers
2. **When** I support propose answers for the consultation and vote for an answer
3. **Then** the consultation should pass

#### Test Steps
- Activate DAO
- Create consultation
- Support consultation
- Propose answers
- Support answers
- Vote for answer
- Check consultation finished with correct answer