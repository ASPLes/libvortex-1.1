:mod:`vortex.Frame` --- PyVortexFrame class: BEEP frame representation
======================================================================

.. currentmodule:: vortex


API documentation for vortex.frame object representing a BEEP frame received.

==========
Module API
==========

.. class:: Frame

   .. attribute:: id

      (Read only attribute) (Number) returns frame unique identifier

   .. attribute:: type

      (Read only attribute) (String) returns the frame type: MSG, RPY, ERR, ANS, NUL and SEQ.

   .. attribute:: msgno

      (Read only attribute) (Number) returns the frame msgno value. It is also accepted msg_no

   .. attribute:: msg_no

      (Read only attribute) (Number) msgno alias 

   .. attribute:: seqno

      (Read only attribute) (Number) returns the frame seqno value. It is also accepted seq_no

   .. attribute:: seq_no

      (Read only attribute) (Number) seqno alias 

   .. attribute:: ansno

      (Read only attribute) (Number) returns the frame ansno value. It is also accepted ans_no

   .. attribute:: ans_no

      (Read only attribute) (Number) ansno alias 

   .. attribute:: more_flag

      (Read only attribute) (True/Flag) returns more flag status on the frame

   .. attribute:: payload_size

      (Read only attribute) (Number) returns the payload size (frame content without MIME headers).

   .. attribute:: content_size

      (Read only attribute) (Number) returns the content size (frame content including MIME headers).

   .. attribute:: payload

      (Read only attribute) (Number) returns frame content without including MIME headers

   .. attribute:: content

      (Read only attribute) (Number) returns frame content including MIME headers
