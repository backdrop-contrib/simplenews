
Description
-----------

Simplenews publishes and sends newsletters to lists of subscribers. Both anonymous and authenticated users can opt-in to different mailing lists. HTML email can be sent by adding Mime mail module.


Requirements
------------

 * For large mailing lists, cron is required.
 * HTML-format newsletters and/or newsletters with file attachments require the mime mail or HMTL mail module.
 * When sending newsletters on regular cron (cron.php), it is important that the base url (settings.php, variable $base_url) is set correctly or links inside the newsletter will not work.
 * Additionally when using Drush to start cron, it is important to use the argument --uri=http://www.example.com


Installation
------------

You can install the module through your Backdrop website's user interface per instructions on https://backdropcms.org/guide/modules or by simply running the `drush en simplenews` command on command line.

Configuration
-------------


* To administer various Simplenews settings go to Configuration > Web services > Newsletter > Settings  (`/admin/config/services/simplenews/settings`).

* Add or edit newsletter categories on Configuration > Web services > Newsletter (`/admin/config/services/simplenews`).

* By default Simplenews creates a new `Simplenews newsletter` content type, however you can enable any other content type to use as a newsletter on Structure > ccontent type > Configure > Publishing settings > Use as simplenews newsletter.

* To enable Simplenews subscription blocks go to Structure > Layouts and choose a layout. Most probably you want to place subscription blocks on Home page or Default layouts. Choose a layout region where you want to place a Simplenews block, click on "Add block" and choose one of `Newsletter: your website newsletter` or `Newsletter: Multi Subscription`.


Usage
-----

1. ENABLE SIMPLENEWS BLOCK

With the Simplenews block users can subscribe to a newsletter. Enable a Simplenews block per Newsletter category on Configuration > Web services Newsletters page.

2. SEND MAILING LISTS

Cron is required to send large mailing lists. If you have a medium or large size mailing list (i.e. more than 500 subscribers) always use cron to send the newsletters.

To use cron:
  * Check the 'Use cron to send newsletters' checkbox.
  * Set the 'Cron throttle' to the number of newsletters send per cron run. Too high values may lead to mail server overload or you may hit hosting restrictions.

Don't use cron:
  * Uncheck the 'Use cron to send newsletters' checkbox.

All newsletters will be sent immediately when saving the node. If not all emails can be sent within the available php execution time, the remainder will be sent by cron. Therefore ALWAYS enable cron.

These settings are found on the Newsletter Settings page under 'Send mail' options at Administer > Configuration > Web Services > Newsletters > Settings > Send mail.

3. (UN)SUBSCRIBE CONFIRMATION

By default the unsubscribe link will direct the user to a confirmation page. Upon confirmation the user is directed to the home page, where a message will be displayed. On the Simplenews subscription admin page you can specify an alternative destination page: Configuration > Web Services > Newsletters > edit newsletter category > Subscription settings

To skip the confirmation page you can add parameters to the subscription URL.
  Example: [simplenews-subscribe-url]/ok

When an alternative destination page has been defined the extra parameters will be added to the destination URL.
  Example: [simplenews-subscriber:subscribe-url]/ok
  Destination: node/123
  Destination URL: node/123/ok

4. SINGLE OR DOUBLE OPT-IN AND OPT-OUT

Every newsletter can be set to be double opt-in/out (default), single opt-in/out, or hidden.

  Double: A confirmation email is sent to confirm the (un)subscribe action. No confirmation is sent when a user is (un)subscribed by the administrator or when the user subscribes when creating an account.
  Single: No confirmation email is sent. (un)subscribe is immediately.
  Hidden: The newsletter is not listed in newsletter lists. Use this for mandatory newsletters. Only administrators or modules can add a user to this mailing list.

Note that single opt-in/out or hidden (forced) subscription is in some countries forbidden by law.

SECURITY NOTICE: a newsletter set to be single opt-in or opt-out is vulnerable to Cross Site Request Forgeries. Email addresses may be (un)subscribed without a notice. Do not use this setting in uncontrolled environments (like the internet!).

5. TIPS

* A subscription page is available at: /newsletter/subscriptions

* The Elysia Cron module (https://backdropcms.org/project/elysia_cron) can be used to start the simplenews cron hook more often than others, so that newsletter are sent faster without decreasing site performance due to long-running cron hooks.


Related modules
------------

 * Elysia Cron
   Allows fine grained control over cron tasks.
   https://backdropcms.org/project/elysia_cron
 * Mailsystem
   Extends Backdrop core mailystem wirh Administrative UI and Developers API.
   https://backdropcms.org/project/mailsystem
 * Maillog
   Captures outgoing mails, helps users debugging Simplenews.
   https://backdropcms.org/project/maillog


License
-------
This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

Credits
-------

This module is based on the Simplenews module for Drupal, originally written and maintained by a large number of contributors, including:

Berdir https://www.drupal.org/u/berdir
miro_dietiker https://www.drupal.org/u/miro_dietiker
Simon Georges https://www.drupal.org/u/simon-georges
Sutharsan https://www.drupal.org/u/sutharsan
AlexisWilke https://www.drupal.org/u/alexiswilke

Current maintainers
-------------------

Simplenews is ported and supported by Backdrop Afficionados at AltaGrade (https://www.altagrade.com):

* Alan Mels (https://github.com/alanmels)
* Alex Shapka (https://github.com/alexshapka)
* Nick Onom (https://github.com/nickonom)
