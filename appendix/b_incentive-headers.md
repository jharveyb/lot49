# Appendix B. Incentive Headers

<table>
  <thead>
    <tr>
      <th style="text-align:left">Header Name</th>
      <th style="text-align:left">Description</th>
      <th style="text-align:left">Nodes</th>
      <th style="text-align:left">Signed Transaction(s)</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="text-align:left">Setup_1</td>
      <td style="text-align:left">Node B signs a transactions similar to the initial Settlement Transaction
        from eltoo [25]. Node B signs to commit to refund the initial channel funding
        from node A. This header from B proposes creating a channel so that A can
        pay B.</td>
      <td style="text-align:left">
        <p>Sender: B</p>
        <p>Receiver: A</p>
      </td>
      <td style="text-align:left">
        <p>Settlement Tx</p>
        <p>Signed by Node B</p>
      </td>
    </tr>
    <tr>
      <td style="text-align:left">Setup_2</td>
      <td style="text-align:left">Node A signs a transaction similar to the initial Funding Transaction
        from eltoo [25] to commit the initial channel funding to a multisig address
        controlled by both Nodes A and B. This transaction can be fully refunded
        because Node B should have already signed a settlement transaction that
        refunds the value back to Node A.</td>
      <td style="text-align:left">
        <p>Sender: A</p>
        <p>Receiver: B</p>
      </td>
      <td style="text-align:left">
        <p>Funding Tx</p>
        <p>Signed by Node A</p>
      </td>
    </tr>
    <tr>
      <td style="text-align:left">Negotiate_1</td>
      <td style="text-align:left">Node A proposes an update to the payment channel balance that increases
        Node B&#x2019;s balance if Node D returns the preimage from the attached
        message m.</td>
      <td style="text-align:left">
        <p>Sender: A</p>
        <p>Receiver: B</p>
        <p>Destination: D</p>
      </td>
      <td style="text-align:left">Update Tx &amp;
        <br />Settlement Tx
        <br />Signed by Node A</td>
    </tr>
    <tr>
      <td style="text-align:left">Negotiate_2</td>
      <td style="text-align:left">Node C proposes an update to the payment channel balance that increases
        Node D&#x2019;s balance if Node D returns the preimage from the attached
        message m. This is similar to Negotiate 1 but Node C proposes to pay the
        value committed by Node A to send the message, less the amount taken by
        relay nodes.</td>
      <td style="text-align:left">Sender: C
        <br />Receiver: D
        <br />Destination: D
        <br />
      </td>
      <td style="text-align:left">Update Tx &amp;
        <br />Settlement Tx
        <br />Signed by Node C</td>
    </tr>
    <tr>
      <td style="text-align:left">Negotiate_3</td>
      <td style="text-align:left">Node D proposes an update to the payment channel balance that increases
        Node C&#x2019;s balance if Node A signs the preimage attached message m.
        This is similar to Negotiate 2 but Node D proposes pays along an existing
        path to reduce incentive hint overhead.
        <br />
      </td>
      <td style="text-align:left">Sender: D
        <br />Receiver: C
        <br />Destination: A</td>
      <td style="text-align:left">Update Tx &amp;
        <br />Settlement Tx
        <br />Signed by Node D</td>
    </tr>
    <tr>
      <td style="text-align:left">Receipt_1</td>
      <td style="text-align:left">Node D proves delivery of message m by returning the preimage contained
        in message m and hashed with the message.</td>
      <td style="text-align:left">Sender: D
        <br />Receiver: C</td>
      <td style="text-align:left">
        <p>Update Tx &amp;
          <br />Settlement Tx &amp;</p>
        <p>H(m)</p>
        <p>Signed by Node D
          <br />
        </p>
      </td>
    </tr>
    <tr>
      <td style="text-align:left">Receipt_2</td>
      <td style="text-align:left">Node C proves delivery of message m to renegotiate the state of their
        channel with B.</td>
      <td style="text-align:left">Sender: C
        <br />Receiver: B
        <br />
      </td>
      <td style="text-align:left">Update Tx &amp;
        <br />Settlement Tx Signed by Node C</td>
    </tr>
    <tr>
      <td style="text-align:left">Close_1</td>
      <td style="text-align:left">Node B proposes to close the channel with node A at the last negotiated
        distribution.</td>
      <td style="text-align:left">Sender: B
        <br />Receiver: A
        <br />
      </td>
      <td style="text-align:left">
        <p>Close Tx</p>
        <p>Signed by Node B</p>
      </td>
    </tr>
    <tr>
      <td style="text-align:left">Close_2</td>
      <td style="text-align:left">Node A agrees to close the channel with node B at the last negotiated
        distribution.</td>
      <td style="text-align:left">Sender: A
        <br />Receiver: B
        <br />
      </td>
      <td style="text-align:left">
        <p>Close Tx</p>
        <p>Signed by Node A</p>
      </td>
    </tr>
  </tbody>
</table>

