SIMPLENEWS
===========

CONTENTS OF THIS FILE
---------------------

 - Introduction
 - Requirements
 - Installation
 - Permissions
 - Usage
 - Sponsors

INTRODUCTION
------------

Simplenews publishes and sends newsletters to lists of subscribers. Both
anonymous and authenticated users can opt-in to different mailing lists.
HTML email can be sent by adding Mime mail module.

TESTED
-----

@todo
This module has NOT BEEN TESTED and is being ported to Backdrop.  It may work.

KNOWN ISSUES
---------------------
@todo


REQUIREMENTS
------------

 * For large mailing lists, cron is required.
 * HTML-format newsletters and/or newsletters with file attachments require the
   mime mail or HMTL mail module.
 * When sending newsletters on regular cron (cron.php), it is important that
   the base url (settings.php, variable $base_url) is set correctly or links
   inside the newsletter will not work. See the Tips (13.) below.
 * Additionally when using Drush to start cron, it is important to use the
   argument --uri=http://www.example.com

INSTALLATION
------------

 1. INSTALL

    Simplenews can be installed via the standard Backdrop installation process
(http://drupal.org/documentation/install/modules-themes/modules-7).

 2. ENABLE THE MODULE

    Enable the module on the Modules admin page.

 3. ACCESS PERMISSION

    Grant the access at the Access control page:
      People > Permissions.

 4. CONFIGURE SIMPLENEWS

    Configure Simplenews on the Simplenews admin pages:
      Configuration > Simplenews.

    Enable new content types to use as newsletter:
      Structure > edit content type > Publishing options

    Add and configure newsletter categories:
      Structure > Web Services > Newsletters > Add newsletter category
      Structure > Web Services > Newsletters > edit newsletter category

 5. ENABLE SIMPLENEWS BLOCK

    With the Simplenews block users can subscribe to a newsletter.

    Enable a Simplenews block per Newsletter category:
      Structure > Newsletters > edit newsletter category

 6. CONFIGURE SIMPLENEWS BLOCK

    Configure the Simplenews block on the Block configuration page. You reach
    this page from Block admin page (Structure > Blocks).
    Click the 'Configure' link of the appropriate simplenews block.

    Permission "subscribe to newsletters" is required to view the subscription
    form in the simplenews block or to view the link to the subscription form.

 7. SIMPLENEWS BLOCK THEMING

    More control over the content of simplenews blocks can be achieved using
    the block theming. Theme your simplenews block by copying
    simplenews-block.tpl.php into your theme directory and edit the content.
    The file is self documented listing all available variables.

    The newsletter block can be themed generally and per newsletter:
      simplenews-block.tpl.php (for all newsletters)
      simplenews-block.tpl--[tid].php (for newsletter series tid)

 8. MULTILINGUAL SUPPORT

    Simplenews supports multilingual newsletters for node translation,
    multilingual taxonomy and url path prefixes.

    When translated newsletter issues are available subscribers receive the
    newsletter in their preferred language (according to account setting).
    Translation module is required for newsletter translation.

    Multilingual taxonomy of 'Localized terms' and 'per language terms' is
    supported. 'per language vocabulary' is not supported.
    I18n-taxonomy module is required.
    Use 'Localized terms' for a multilingual newsletter. Taxonomy terms are
    translated and translated newsletters are each tagged with the same
    (translated) term. Subscribers receive the newsletter in the preferred
    language set in their account settings or in the site default language.
    Use 'per language terms' for mailing lists each with a different language.
    Newsletters of different language each have their own tag and own list of
    subscribers.

    Path prefixes are added to footer message according to the subscribers
    preferred language.

    The preferred language of anonymous users is set based on the interface
    language of the page they visit for subscription. Anonymous users can NOT
    change their preferred language. Users with an account on the site will be
    subscribed with the preferred language as set in their account settings.

    The confirmation mails can be translated by enableding the Simplenews
    variables at:
      Home > Administration > Configuration > Regional and language > Multilingual settings > Variables
    Afterwards, the mail subject and body can be entered for every enabled
    language.

9.  NEWSLETTER THEMING

    You can customize the theming of newsletters. Copy any of the *.tpl.php
    files from the simplenews module directory to your theme directory. Both
    general and by-newsletter theming can be performed.
    Theme newsletter body:
      simplenews-newsletter-body.tpl.php (for all newsletters)
      simplenews-newsletter-body--[tid].tpl.php
      simplenews-newsletter-body--[view mode].tpl.php
      simplenews-newsletter-body--[tid]--[view mode].tpl.php

      [tid]: Machine readable name of the newsletter category
      [view mode]: 'email-plain', 'email-html', 'email-textalt'
      Example:
        simplenews-newsletter-body--1--email-plain.tpl.php

    Theme newsletter footer:
      simplenews-newsletter-footer.tpl.php (for all newsletters)
      simplenews-newsletter-footer--[tid].tpl.php
      simplenews-newsletter-footer--[view mode].tpl.php
      simplenews-newsletter-footer--[tid]--[view mode].tpl.php

      [tid]: Machine readable name of the newsletter category
      [view mode]: 'email-plain', 'email-html', 'email-textalt'
      Example:
        simplenews-newsletter-footer--1--email-plain.tpl.php

    The template files are self documented listing all available variables.
    Depending on how the mails are sent (e.g. how cron is triggered), either the
    default or the admin theme might be used, if one has been configured.
    To prevent this, Simplenews supports the mail theme setting from the
    mailsystem module (http://drupal.org/project/mailsystem). Install it, choose
    the mail theme and the newsletter templates from that theme will be used no
    matter which other themes are enabled.

    Using the fields Display settings each field of a simplenews newsletter can
    be displayed or hidden in 'plain text', 'HTML' and 'HTML text alternative'
    format. You find these settings at:
      Structure > Content types > Manage display
    Enable the view modes you want to configure and configure their display.

10. SEND MAILING LISTS

    Cron is required to send large mailing lists.
    If you have a medium or large size mailing list (i.e. more than 500
    subscribers) always use cron to send the newsletters.

    To use cron:
     * Check the 'Use cron to send newsletters' checkbox.
     * Set the 'Cron throttle' to the number of newsletters send per cron run.
       Too high values may lead to mail server overload or you may hit hosting
       restrictions. Contact your host.

    Don't use cron:
     * Uncheck the 'Use cron to send newsletters' checkbox.
       All newsletters will be sent immediately when saving the node. If not
       all emails can be sent within the available php execution time, the
       remainder will be sent by cron. Therefore ALWAYS enable cron.

    These settings are found on the Newsletter Settings page under
    'Send mail' options at:
      Administer > Configuration > Web Services > Newsletters > Settings > Send mail.

11. (UN)SUBSCRIBE CONFIRMATION

    By default the unsubscribe link will direct the user to a confirmation page.
    Upon confirmation the user is directed to the home page, where a message
    will be displayed. On the Simplenews subscription admin page you can
    specify an alternative destination page.
      Structure > Configuration > Web Services > Newsletters > edit newsletter category > Subscription settings

    To skip the confirmation page you can add parameters to the subscription URL.
      Example: [simplenews-subscribe-url]/ok
    When an alternative destination page has been defined the extra parameters
    will be added to the destination URL.
      Example: [simplenews-subscriber:subscribe-url]/ok
      Destination: node/123
      Destination URL: node/123/ok

 12. SINGLE OR DOUBLE OPT-IN AND OPT-OUT

    Every newsletter can be set to be double opt-in/out (default), single
    opt-in/out, or hidden.

    Double: A confirmation email is sent to confirm the (un)subscribe action.
            No confirmation is sent when a user is (un)subscribed by the
            administrator or when the user subscribes when creating an account.
    Single: No confirmation email is sent. (un)subscribe is immediately.
    Hidden: The newsletter is not listed in newsletter lists. Use this for
    mandatory newsletters. Only administrators or modules can add a user to this
    mailing list.

    Note that single opt-in/out or hidden (forced) subscription is in some
    countries forbidden by law.

    SECURITY NOTICE: a newsletter set to be single opt-in or opt-out is
    vulnerable to Cross Site Request Forgeries. Email addresses may be
    (un)subscribed without a notice. Do not use this setting in uncontrolled
    environments (like the internet!).

 13. TIPS
    A subscription page is available at: /newsletter/subscriptions

    The Elysia Cron module can be used
    to start the simplenews cron hook more often than others, so that newsletter
    are sent faster without decreasing site performance due to long-running cron
    hooks.

    If your unsubscribe URL looks like:
      http://newsletter/confirm/remove/8acd182182615t632
    instead of:
      http://www.example.com/newsletter/confirm/remove/8acd182182615t632
    You should change the base URL in the settings.php file from
      #  $base_url = 'http://www.example.com';  // NO trailing slash!
    to
      $base_url = 'http://www.example.com';  // NO trailing slash!


PERMISSIONS
------------

@todo


USAGE
-----
Sending nodes as newsletters to subscribers
Multiple newsletter categories with separate settings
Per category and multi-signup Blocks and Pages
Subscriber management including mass-subscription and export
Optional E-mail confirmations for anonymous users
Customizable newsletter templates
Support for HTML (including text alternative) newsletter when used in combination with a supported mail system module
Views and Rules integration
Support for multi-language newsletters



License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.

Maintainers
-----------

- seeking

Current Maintainers on Drupal:

Simplenews is currently maintained by MD Systems (by Miro Dietiker, Berdir, s_leu) with the help of Simon Georges of Makina-Corpus.
Originally written by DriesK, later maintained by RobRoy and Sutharsan.

Berdir <https://www.drupal.org/u/berdir>
miro_dietiker <https://www.drupal.org/u/miro_dietiker>
Simon Georges <https://www.drupal.org/u/simon-georges>
Sutharsan <https://www.drupal.org/u/sutharsan>
AlexisWilke <https://www.drupal.org/u/alexiswilke>

If Simplenews is critical for your business, please contact us and help support the ongoing further development of this module. Your donations are appreciated. Also if you like to join the Simplenews maintainer team, don't hesitate to contact us. We are looking forward to hearing from you.

Ported to Backdrop by:

 - biolithic <https://github.com/biolithic>
