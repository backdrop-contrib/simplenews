
Simplenews
==========

Send newsletters to subscribers, with opt-in/out controls and per-newsletter categories. Supports HTML mail via the [Mimemail module](https://github.com/backdrop-contrib/mimemail).

Requirements
------------

- Cron is recommended for any list of real size.
- HTML email and attachments require Mimemail.
- Set `$base_url` correctly when sending from cron so links render properly.

Installation
------------

- UI: Add the module via the Backdrop module installer.
- CLI: `bee dl simplenews && bee en simplenews`

Configuration
-------------

- Settings: Configuration → Web services → Newsletter → Settings (`/admin/config/services/simplenews/settings`).
- Newsletters: Configuration → Web services → Newsletter (`/admin/config/services/simplenews`).
- Content types: Any type can be marked “Use as Simplenews newsletter” under Publishing settings; a Simplenews newsletter type is provided by default.
- Blocks: Place subscription blocks via Structure → Layouts → Add block → “Newsletter: …”.

Sending
-------

- Use cron for medium/large lists; set “Use cron to send newsletters” and choose a throttle.
- Without cron, sends happen on save; any remaining messages still rely on cron to finish. Cron is strongly recommended.

Subscriptions
-------------

- Default unsubscribe flow shows a confirmation page; destination can be changed per newsletter category.
- Opt-in/out modes per newsletter: double (default), single, or hidden (admin-only/forced). Be mindful of legal and CSRF risks with single/hidden modes.
- Public subscription page: `/newsletter/subscriptions`.

Tips
----

- Elysia Cron can schedule Simplenews more frequently: <https://backdropcms.org/project/elysia_cron>
- Maillog captures outgoing mail for debugging: <https://backdropcms.org/project/maillog>
- Mailsystem provides mail-system configuration UI/API: <https://backdropcms.org/project/mailsystem>

Credits
-------

Simplenews is based on the Drupal module by many contributors including Berdir, miro_dietiker, Simon Georges, Sutharsan, and AlexisWilke. Ported to Backdrop by AltaGrade (<https://www.altagrade.com>); current maintainer: [Alan Mels](https://github.com/alanmels).
