# Appendix C. Implied Transactions / Input Scripts

<table>
  <thead>
    <tr>
      <th style="text-align:left">Transaction Name</th>
      <th style="text-align:left">Description</th>
      <th style="text-align:left">Nodes</th>
      <th style="text-align:left">Condition(s)</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="text-align:left">Setup Tx</td>
      <td style="text-align:left">Spend from &#x2018;Funding&#x2019; UTXO to &apos;Setup&apos; UTXO when
        a new channel is funded. Node A will not sign this until they have a signed
        Refund Tx from Node B.</td>
      <td style="text-align:left">Sender: A
        <br />Receiver: B
        <br />
      </td>
      <td style="text-align:left">A &amp; B sign</td>
    </tr>
    <tr>
      <td style="text-align:left">Refund Tx</td>
      <td style="text-align:left">Spend from &#x2018;Setup&#x2019; UTXO to &#x2018;Funding&#x2019; UTXOs
        after some delay. Specifically created when a channel is first created.</td>
      <td
      style="text-align:left">Sender: A
        <br />Receiver: B</td>
        <td style="text-align:left">
          <p>A &amp; B sign;</p>
          <p>Valid after a delay relative to when the transaction it spends from is
            committed.</p>
        </td>
    </tr>
    <tr>
      <td style="text-align:left">Update Tx</td>
      <td style="text-align:left">Spend from &#x2018;Setup&#x2019; or older &#x2018;Update&#x2019; UTXO
        to &apos;Update&apos; UTXO with newer state. Occurs immediately when this
        transaction is committed.</td>
      <td style="text-align:left">Sender: A
        <br />Receiver: B</td>
      <td style="text-align:left">
        <p>A &amp; B sign;</p>
        <p>Current channel state &gt; channel state of transaction it spends from</p>
      </td>
    </tr>
    <tr>
      <td style="text-align:left">Settlement Tx</td>
      <td style="text-align:left">Spend from &#x2018;Setup&#x2019; or &#x2018;Update&#x2019; UTXO to final
        single signature &#x2018;Funding&#x2019; UTXOs after some delay. Encodes
        a specific balance division.</td>
      <td style="text-align:left">
        <p>Sender: A
          <br />Receiver: B</p>
        <p>Destination: D</p>
      </td>
      <td style="text-align:left">A &amp; B sign;
        <br />Valid after a delay relative to when the transaction it spends from is
        committed;
        <br />D signs hash of Message</td>
    </tr>
    <tr>
      <td style="text-align:left">Close Tx</td>
      <td style="text-align:left">Unconditionally move funds from the most recent &#x2018;Update&#x2019;
        UTXO to a set of balances at final single key &#x2018;Funding&#x2019; UTXOs.</td>
      <td
      style="text-align:left">Sender: A
        <br />Receiver: B</td>
        <td style="text-align:left">A &amp; B sign
          <br />
        </td>
    </tr>
  </tbody>
</table>