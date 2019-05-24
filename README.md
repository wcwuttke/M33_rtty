<h1 style="text-align: center;">M33_rtty</h1>
<p>An Arduino project that converts a Model 33 Teletype into an improved version of a Model 32 Teletype. This project was a "great experiment" to make my Model 33 work with my HAL ST-8000 HF Modem.</p>
<p>Features:</p>
<ol>
<li>Model 32 mode converts ASCII to Baudot and vice-versa</li>
<li>Straight Model 33 mode</li>
<li>45, 50, 75, 100 baud Baudot</li>
<li>Reader Control feature controlled by CTS or internal buffer. Makes possible the use of the Paper Tape Reader when operating at 45 baud.</li>
<li>Unshift on space</li>
<li>Local Echo</li>
<li>Convert LF to CRLF</li>
<li>Line wrap at 68 or 72 characters</li>
<li>Macros. 10 programmable macros are available using the Escape key plus a number 0-9 from the M33 keyboard.</li>
<li>Debug mode. Prints a '&lt;' for figures and a '&gt;' for letters</li>
<li>All features programmable via the Arduino USB programming port at 9600 bps, using the Escape key.</li>
</ol>