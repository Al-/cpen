C-Pen is a nice, small hand-held scanner commonly used to scan short lines of
text, e.g., to copy quotes from a book into a document or from invoice slips
to online banking. See http://cpen.com

Unluckily there is no Linux support by the manufacturer.

Reading the USB data is easy, converting the image to text, too. The step in
between: decoding a proprietary encoded image. This library attempts to do the
latter, based on proprietary code kindly provided by the manufacturer (thus,
the library is closed-source). See cpen_backend.h for further details.

The remaining files in this directory are an example of a Qt-based application
that uses the provided library. BTW, the library is written in C and C++ (no Qt).

