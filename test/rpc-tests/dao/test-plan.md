# Navcoin DAO Test Plan

This file is a map of the tests scenarios which will be covered to ensure the DAO is fully tested before release. This is currently a work in progress, so if you notice there are missing scenarios please add them to this file following the same format/template. If you want to create a test based on a scenario here then please go ahead and claim it. Just make sure you claim and check off any scenarios which you write so others are aware of what is still todo.

## Scenarios

### Scenario 001

| ID          |  DAO_003 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should create a proposal and let it expire |
| File        | `./001-proposal-expired.py`

1. **Given** I have created a proposal
2. **When** The proposal is not voted on
3. **Then** the proposal should expire

#### Test Steps
- Activate DAO
- Donate to the DAO
- Create a proposal
- Do not cast any votes
- End the full round of voting cycles
- Check proposal expired

### Scenario 002

| ID          |  DAO_002 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should create a proposal and the network should reject the proposal |
| File        | `./002-proposal-rejected.py`

1. **Given** I have created a proposal
2. **When** The proposal receives the consensus of `no` votes
3. **Then** the proposal should be rejected

#### Test Steps
- Activate DAO
- Donate to the DAO
- Create a proposal
- Votes `no` on the proposal
- End the full round of voting cycles
- Check proposal is rejected

### Scenario 003

| ID          |  DAO_003 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should create a proposal and the network should accept the proposal |
| File        | `./003-proposal-accepted.py`

1. **Given** I have created a proposal
2. **When** The proposal receives the consensus of `yes` votes
3. **Then** the proposal should be accepted

#### Test Steps
- Activate DAO
- Donate to the DAO
- Create a proposal
- Votes `yes` on the proposal
- End the full round of voting cycles
- Check proposal is rejected

### Scenario 004

| ID          |  DAO_004 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should fail to create a payment request for an expired proposal |
| File        | `./004-proposal-expired-preq.py`

1. **Given** I have an expired proposal
2. **When** A payment request is made
3. **Then** the payment request should not exist

#### Test Steps
- Activate DAO
- Donate to the DAO
- Create a proposal
- Do not cast any votes
- End the full round of voting cycles
- Submit a payment request
- Check payment request does not exist

### Scenario 005

| ID          |  DAO_005 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should fail to create a payment request for a rejected proposal |
| File        | `./005-proposal-expired-preq.py`

1. **Given** I have a rejected proposal
2. **When** A payment request is made
3. **Then** the payment request should not exist

#### Test Steps
- Activate DAO
- Donate to the DAO
- Create a proposal
- Votes `no` on the proposal
- End the full round of voting cycles
- Submit a payment request
- Check payment request does not exist

### Scenario 006

| ID          |  DAO_006 |
| ----------- | -----: |
| Reporter    | @proletesseract |
| Author      | @proletesseract |
| Satus       | Done |
| Description | It should create a payment request for an accepted proposal |
| File        | `./006-proposal-accepted-preq.py`

1. **Given** I have an accepted proposal
2. **When** A payment request is made
3. **Then** the payment request should exist

#### Test Steps
- Activate DAO
- Donate to the DAO
- Create a proposal
- Votes `yes` on the proposal
- End the full round of voting cycles
- Submit a payment request
- Check payment request exists
