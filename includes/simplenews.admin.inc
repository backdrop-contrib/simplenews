<?php

/**
 * @file
 * Newsletter admin, subscription admin, SimpleNews settings.
 *
 * @ingroup SimpleNews
 */

/**
 * Menu callback: Admin form for sent and draft newsletters.
 *
 * @see simplenews_admin_newsletter_submit()
 */
function simplenews_admin_newsletter_issues($form, &$form_state, $action = 'sent') {
  // @todo Fix the delete operation
  $form['filter'] = simplenews_issue_filter_form();
  $form['#submit'][] = 'simplenews_issue_filter_form_submit';
  $form['filter']['#theme'] = 'simplenews_filter_form';
  $form['admin'] = simplenews_admin_issues();

  return $form;
}

/**
 * Generate issue filters.
 */
function simplenews_issue_filters() {
  // Newsletter filter.
  $filters['category'] = array(
    'title' => t('Subscribed to'),
    'options' => array(
      'all' => t('All newsletters'),
      'tid-0' => t('Unassigned newsletters'),
    ),
  );
  foreach (simplenews_category_list() as $tid => $name) {
    $filters['category']['options']['tid-' . $tid] = $name;
  }

  return $filters;
}

/**
 * Return form for issue filters.
 *
 * @see simplenews_issue_filter_form_submit()
 */
function simplenews_issue_filter_form() {
  // Current filter selections in $session var; stored at form submission
  // Example: array('category' => 'all')
  $session = $_SESSION['simplenews_issue_filter'] ?? _simplenews_issue_filter_default();
  $filters = simplenews_issue_filters();

  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Show only newsletters which'),
  );

  // Filter values are default.
  $form['filters']['category'] = array(
    '#type' => 'select',
    '#title' => check_plain($filters['category']['title']),
    '#options' => $filters['category']['options'],
    '#default_value' => $session['category'],
  );
  $form['filters']['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#prefix' => '<span class="spacer" />',
  );
  // Add Reset button if filter is in use.
  if ($session != _simplenews_issue_filter_default()) {
    $form['filters']['buttons']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
    );
  }

  return $form;
}

/**
 * Helper function: returns issue filter default settings.
 */
function _simplenews_issue_filter_default() {
  return array(
    'category' => 'all',
  );
}

/**
 * Form submit callback for the newsletter issue filter.
 */
function simplenews_issue_filter_form_submit($form, &$form_state) {
  switch ($form_state['values']['op']) {
    case t('Filter'):
      $_SESSION['simplenews_issue_filter'] = array(
        'category' => $form_state['values']['category'],
      );
      break;

    case t('Reset'):
      $_SESSION['simplenews_issue_filter'] = _simplenews_issue_filter_default();
      break;
  }
}

/**
 * Form builder: Builds a list of newsletters with operations.
 *
 * @see simplenews_admin_issues_validate()
 * @see simplenews_admin_issues_submit()
 */
function simplenews_admin_issues() {
  // Build an 'Update options' form.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $options = array();
  foreach (module_invoke_all('simplenews_issue_operations') as $operation => $array) {
    $options[$operation] = $array['label'];
  }
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => 'activate',
  );
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#submit' => array('simplenews_admin_issues_submit'),
    '#validate' => array('simplenews_admin_issues_validate'),
  );

  if ($last_cron = state_get('simplenews_last_cron')) {
    $last_sent = state_get('simplenews_last_sent');
    $form['last_sent'] = array(
      '#markup' => '<p>' . format_plural($last_sent, 'Last batch: 1 mail sent at !time.', 'Last batch: !count mails sent at !time.', array('!time' => format_date($last_cron, 'small'), '!count' => $last_sent)) . "</p>\n",
    );
  }
  // Table header. Used as tablesort default.
  $header = array(
    'title' => array(
      'data' => t('Title'),
      'field' => 'n.title',
    ),
    'category' => array(
      'data' => t('Newsletter category'),
      'field' => 'sc.name',
    ),
    'created' => array(
      'data' => t('Created'),
      'field' => 'n.created',
      'sort' => 'desc',
    ),
    'published' => array('data' => t('Published')),
    'sent' => array('data' => t('Sent')),
    'subscribers' => array('data' => t('Subscribers')),
    'operations' => array('data' => t('Operations')),
  );

  $query = db_select('node', 'n')->extend('PagerDefault')->extend('TableSort');
  simplenews_build_issue_filter_query($query);
  $query->innerJoin('simplenews_newsletter', 'sn', 'n.nid = sn.nid');
  $query->leftJoin('simplenews_category', 'sc', 'sn.tid = sc.tid');
  $query->leftJoin('taxonomy_term_data', 't', 'sc.tid = t.tid');
  // $query->leftJoin('users', 'u', 'ss.uid = u.uid');
  $query->fields('n', array('nid', 'title', 'created', 'status'))
    ->fields('sn', array('tid'))
    ->fields('t', array('name'))
    ->limit(30)
    ->orderByHeader($header);
  $query->addField('sn', 'status', 'sent_status');

  $options = array();

  module_load_include('inc', 'simplenews', 'includes/simplenews.mail');
  foreach ($query->execute() as $issue) {
    $categories = simplenews_category_list();
    $subscriber_count = simplenews_count_subscriptions($issue->tid);
    $pending_count = simplenews_count_spool(array('nid' => $issue->nid));
    $send_status = $issue->sent_status == SIMPLENEWS_STATUS_SEND_PENDING ? $subscriber_count - $pending_count : theme('simplenews_status', array('source' => 'sent', 'status' => $issue->sent_status));

    $options[$issue->nid] = array(
      'title' => l($issue->title, 'node/' . $issue->nid),
      'category' => $issue->tid && isset($categories[$issue->tid]) ? $categories[$issue->tid] : t('- Unassigned -'),
      'created' => format_date($issue->created, 'small'),
      'published' => theme('simplenews_status', array('source' => 'published', 'status' => $issue->status)),
      'sent' => $send_status,
      'subscribers' => $subscriber_count,
      'operations' => l(t('edit'), 'node/' . $issue->nid . '/edit', array('query' => backdrop_get_destination())),
    );
  }

  $form['issues'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No newsletters available.'),
  );

  $form['pager'] = array('#theme' => 'pager');

  return $form;
}

/**
 * Implements hook_simplenews_issue_operations().
 */
function simplenews_simplenews_issue_operations() {
  // @todo future ideas: 'pause' => t('Pause sending'), 'resume' => t('Resume sending'), 'cancel' => t('Cancel sending'), 'publish' => t('Publish'), 'unpublish' => t('Unpublish'));
  $operations = array(
    'activate' => array(
      'label' => t('Send'),
      'callback' => 'simplenews_issue_send',
    ),
  );
  return $operations;
}

/**
 * Form validate callback for the issue list operations.
 */
function simplenews_admin_issues_validate($form, &$form_state) {
  if (isset($form_state['input']['operation'])) {
    $nids = array_keys(array_filter($form_state['input']['issues']));
    if (empty($nids)) {
      form_set_error('', t('No items selected.'));
    }
  }
}

/**
 * Form submit callback for the issue operations.
 */
function simplenews_admin_issues_submit($form, &$form_state) {
  // Call operation functions as defined in hook_simplenews_issue_operations().
  $operations = module_invoke_all('simplenews_issue_operations');
  $operation = $operations[$form_state['values']['operation']];
  // Filter out unchecked list issues.
  $nids = array_filter($form_state['values']['issues']);
  if ($function = $operation['callback']) {
    // Add in callback arguments if present.
    if (isset($operation['callback arguments'])) {
      $args = array_merge(array($nids), $operation['callback arguments']);
    }
    else {
      $args = array($nids);
    }
    call_user_func_array($function, $args);
  }
  else {
    // We need to rebuild the form to go to a second step. For example, to
    // show the confirmation form for the deletion of nodes.
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Callback to send newsletters.
 */
function simplenews_issue_send($nids) {
  $sent_nodes = array();
  foreach (node_load_multiple($nids) as $node) {
    $newsletter = simplenews_newsletter_load($node->nid);
    if ($newsletter->status != SIMPLENEWS_STATUS_SEND_NOT) {
      continue;
    }

    if ($node->status == NODE_NOT_PUBLISHED) {
      simplenews_newsletter_update_sent_status($node, SIMPLENEWS_COMMAND_SEND_PUBLISH);
      backdrop_set_message(t('Newsletter %title is unpublished and will be sent on publish.', array('%title' => $node->title)));
      continue;
    }

    simplenews_add_node_to_spool($node);
    $sent_nodes[$node->nid] = $node->title;
  }

  // If there were any newsletters sent, display a message.
  if (!empty($sent_nodes)) {
    $conditions = array('nid' => array_keys($sent_nodes));
    // Attempt to send immediately, if configured to do so.
    if (simplenews_mail_attempt_immediate_send($conditions)) {
      backdrop_set_message(t('Sent the following newsletters: %titles.', array('%titles' => implode(', ', $sent_nodes))));
    }
    else {
      backdrop_set_message(t('The following newsletter are now pending: %titles.', array('%titles' => implode(', ', $sent_nodes))));
    }
  }
}

/**
 * Apply filters for subscription filters based on session.
 *
 * @param $query
 *   A SelectQuery to which the filters should be applied.
 */
function simplenews_build_issue_filter_query(SelectQueryInterface $query) {
  if (isset($_SESSION['simplenews_issue_filter'])) {
    foreach ($_SESSION['simplenews_issue_filter'] as $key => $value) {
      switch ($key) {
        case 'list':
        case 'category':
          if ($value != 'all') {
            [$key, $value] = explode('-', $value, 2);
            $query->condition('sn.' . $key, $value);
          }
          break;
      }
    }
  }
}

/**
 * Menu callback: list admin form with list of available list categories.
 *
 * @ingroup forms
 * @see simplenews_admin_newsletter_categories_submit()
 * @see theme_simplenews_admin_newsletter_categories()
 */
function simplenews_admin_categories() {
  $form['#tree'] = TRUE;
  if ($categories = simplenews_categories_load_multiple(array(), array('show_all' => TRUE))) {
    foreach ($categories as $category) {
      $form[$category->tid]['#category'] = $category;
      $form[$category->tid]['name'] = array('#markup' => check_plain(_simplenews_newsletter_name($category)));
      $form[$category->tid]['count'] = array('#markup' => simplenews_count_subscriptions($category->tid));
      $form[$category->tid]['optinout'] = array('#markup' => ucfirst($category->opt_inout));

      $form[$category->tid]['edit'] = array(
        '#type' => 'dropbutton',
        '#links' => array(
          'edit' => array(
            'title' => t('Edit category'),
            'href' => "admin/config/services/simplenews/categories/$category->tid/edit",
          ),
          'delete' => array(
            'title' => t('Delete'),
            'href' => "admin/config/services/simplenews/categories/$category->tid/delete",
          ),
        ),
      );
    }
  }

  return $form;
}

/**
 * Form builder function, display a list of SimpleNews categories.
 *
 * @ingroup theming
 */
function theme_simplenews_admin_categories($variables) {
  $form = $variables['form'];

  $rows = array();

  foreach (element_children($form) as $key) {
    if (isset($form[$key]['name'])) {
      $category = &$form[$key];

      $row = array();
      $row[] = backdrop_render($category['name']);
      $row[] = backdrop_render($category['count']);
      $row[] = backdrop_render($category['optinout']);
      if (isset($category['weight'])) {
        $category['weight']['#attributes']['class'] = array('simplenews-category-weight');
        $row[] = backdrop_render($category['weight']);
      }
      $row[] = backdrop_render($category['edit']);
      $rows[] = array(
        'data' => $row,
      );
    }
  }

  $header = array(t('Newsletter category name'));
  $header[] = t('Subscribers');
  $header[] = t('Opt In/Out status');
  if (isset($form['submit'])) {
    $header[] = t('Weight');
    backdrop_add_tabledrag('newsletter-category', 'order', 'self', 'simplenews-category-weight');
  }
  $header[] = array(
    'data' => t('Operations'),
    'colspan' => '3',
  );

  return theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No newsletter categories. <a href="@link">Add category</a>.', array('@link' => url('admin/config/services/simplenews/add'))), 'attributes' => array('id' => 'newsletter-category'))) . backdrop_render_children($form);
}

/**
 * Menu callback: newsletter admin form for newsletter add/edit.
 *
 * @see simplenews_admin_category_form_validate()
 * @see simplenews_admin_category_form_submit()
 */
function simplenews_admin_category_form($form, &$form_state, $edit = array()) {
  $config = config('simplenews.settings');

  if (!is_array($edit)) {
    $edit = (array) $edit;
  }

  $edit += array(
    'tid' => 0,
    'name' => '',
    'description' => '',
    'weight' => '0',
    'new_account' => 'none',
    'opt_inout' => 'double',
    'block' => 1,
    'format' => $config->get('format'),
    'priority' => $config->get('priority'),
    'receipt' => $config->get('receipt'),
    'from_name' => $config->get('from_name'),
    'from_address' => $config->get('from_address'),
    'email_subject' => '[[simplenews-category:name]] [node:title]',
    'hyperlinks' => 1,
  );

  $form['#category'] = (object) $edit;
  // Check whether we need a deletion confirmation form.
  if (isset($form_state['confirm_delete']) && isset($form_state['values']['tid'])) {
    $category = simplenews_category_load($form_state['values']['tid']);
    return simplenews_admin_category_delete($form, $form_state, $category);
  }
  $form['tid'] = array(
    '#type' => 'value',
    '#value' => $edit['tid'],
  );
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => $edit['name'],
    '#maxlength' => 255,
    '#required' => TRUE,
  );
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => $edit['description'],
  );
  $form['weight'] = array(
    '#type' => 'hidden',
    '#value' => $edit['weight'],
  );
  $form['subscription'] = array(
    '#type' => 'fieldset',
    '#title' => t('Subscription settings'),
    '#collapsible' => FALSE,
  );

  // Subscribe at account registration time.
  $options = array(
    'none' => t('None'),
    'on' => t('Default on'),
    'off' => t('Default off'),
    'silent' => t('Silent'),
  );
  $form['subscription']['new_account'] = array(
    '#type' => 'select',
    '#title' => t('Subscribe new account'),
    '#options' => $options,
    '#default_value' => $edit['new_account'],
    '#description' => t('None: This newsletter is not listed on the user registration page.<br />Default on: This newsletter is listed on the user registration page, is selected by default, and also applies to users created programmatically.<br />Default off: This newsletter is listed on the user registration page and is not selected by default.<br />Silent: A new user is automatically subscribed (no checkbox is shown on the registration form).'),
  );

  // Type of (un)subsribe confirmation.
  $options = array(
    SIMPLENEWS_OPT_INOUT_HIDDEN => t('Hidden'),
    SIMPLENEWS_OPT_INOUT_SINGLE => t('Single'),
    SIMPLENEWS_OPT_INOUT_DOUBLE => t('Double'),
  );
  $form['subscription']['opt_inout'] = array(
    '#type' => 'select',
    '#title' => t('Opt-in/out method'),
    '#options' => $options,
    '#default_value' => $edit['opt_inout'],
    '#description' => t('Hidden: This newsletter does not appear on subscription forms. No unsubscription footer in newsletter.<br /> Single: Users are (un)subscribed immediately, no confirmation email is sent.<br />Double: When (un)subscribing at a subscription form, anonymous users receive an (un)subscription confirmation email. Authenticated users are (un)subscribed immediately.'),
  );

  // Provide subscription block for this category.
  $form['subscription']['block'] = array(
    '#type' => 'checkbox',
    '#title' => t('Subscription block'),
    '#default_value' => $edit['block'],
    '#description' => t('A subscription block will be provided for this newsletter category. Anonymous and authenticated users can subscribe and unsubscribe using this block.'),
  );

  $form['email'] = array(
    '#type' => 'fieldset',
    '#title' => t('Email settings'),
    '#collapsible' => FALSE,
  );
  // Hide format selection if there is nothing to choose.
  // The default format is plain text.
  $format_options = simplenews_format_options();
  if (count($format_options) > 1) {
    $form['email']['format'] = array(
      '#type' => 'radios',
      '#title' => t('Email format'),
      '#default_value' => $edit['format'],
      '#options' => $format_options,
    );
  }
  else {
    $form['email']['format'] = array(
      '#type' => 'hidden',
      '#value' => key($format_options),
    );
    $form['email']['format_text'] = array(
      '#markup' => t('Newsletter emails will be sent in %format format.', array('%format' => $edit['format'])),
    );
  }

  $form['email']['priority'] = array(
    '#type' => 'select',
    '#title' => t('Email priority'),
    '#default_value' => $edit['priority'],
    '#options' => simplenews_get_priority(),
  );
  $form['email']['receipt'] = array(
    '#type' => 'checkbox',
    '#title' => t('Request receipt'),
    '#return_value' => 1,
    '#default_value' => $edit['receipt'],
  );

  // Email sender name.
  $form['simplenews_sender_information'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sender information'),
    '#collapsible' => FALSE,
  );
  $form['simplenews_sender_information']['from_name'] = array(
    '#type' => 'textfield',
    '#title' => t('From name'),
    '#size' => 60,
    '#maxlength' => 128,
    '#default_value' => $edit['from_name'],
  );
  // Email from address.
  $form['simplenews_sender_information']['from_address'] = array(
    '#type' => 'textfield',
    '#title' => t('From email address'),
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => TRUE,
    '#default_value' => $edit['from_address'],
  );

  // Email subject.
  $form['simplenews_subject'] = array(
    '#type' => 'fieldset',
    '#title' => t('Newsletter subject'),
    '#collapsible' => FALSE,
  );
  if (module_exists('token')) {
    $form['simplenews_subject']['token_help'] = array(
      '#title' => t('Replacement patterns'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['simplenews_subject']['token_help']['browser'] = array(
      '#theme' => 'token_tree',
      '#token_types' => array('simplenews-category', 'node', 'simplenews-subscriber'),
    );
  }

  $form['simplenews_subject']['email_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Email subject'),
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => TRUE,
    '#default_value' => $edit['email_subject'],
  );

  // Type of hyperlinks.
  $form['simplenews_hyperlinks'] = array(
    '#type' => 'fieldset',
    '#title' => t('HTML to text conversion'),
    '#collapsible' => FALSE,
    '#description' => t('When your newsletter is sent as plain text, these options will determine how the conversion to text is performed.'),
  );
  $form['simplenews_hyperlinks']['hyperlinks'] = array(
    '#type' => 'radios',
    '#title' => t('Hyperlink conversion'),
    '#options' => array(t('Append hyperlinks as a numbered reference list'), t('Display hyperlinks inline with the text')),
    '#default_value' => $edit['hyperlinks'],
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 50,
  );

  if ($edit['tid']) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 55,
    );
  }
  return $form;
}

/**
 * Form validation callback for a newsletter category form.
 */
function simplenews_admin_category_form_validate($form, &$form_state) {
  if ($form_state['clicked_button']['#value'] != t('Delete')) {

    // Check for valid email address.
    if (!valid_email_address($form_state['values']['from_address'])) {
      form_set_error('from_address', t("The sender's email address you supplied is not valid."));
    }
  }
}

/**
 * Form submit callback for a newsletter category form.
 */
function simplenews_admin_category_form_submit($form, &$form_state) {

  $op = $form_state['values']['op'] ?? '';
  if ($op == t('Delete')) {
    $form_state['redirect'] = 'admin/config/services/simplenews/categories/' . $form_state['values']['tid'] . '/delete';
    return;
  }

  $category = (object) $form_state['values'];

  // Make sure the default newsletter vocabulary exists.
  $vocabulary = taxonomy_vocabulary_load('newsletter');
  if (empty($vocabulary)) {
    $vocabulary = new TaxonomyVocabulary();
    $vocabulary->name = 'Newsletter';
    $vocabulary->machine_name = "newsletter";
    $vocabulary->description = t('SimpleNews newsletter categories.');
    $vocabulary->module = 'taxonomy';
    taxonomy_vocabulary_save($vocabulary);
  }

  // Add a term to the vocabulary.
  $term = entity_create('taxonomy_term', array(
    'name' => $form_state['values']['name'],
    'description' => $form_state['values']['description'],
    'vocabulary' => taxonomy_vocabulary_load('newsletter')->machine_name,
    'weight' => $form_state['values']['weight'],
    'tid' => $form_state['values']['tid'],
  ));
  taxonomy_term_save($term);

  $category->tid = $term->tid;

  switch (simplenews_category_save($category)) {
    case SAVED_NEW:
      backdrop_set_message(t('Created new newsletter category %name.', array('%name' => _simplenews_newsletter_name($category))));
      watchdog('simplenews', 'Created new newsletter category %name.', array('%name' => _simplenews_newsletter_name($category), WATCHDOG_NOTICE, l(t('edit'), 'admin/config/services/simplenews/categories/' . $category->tid . '/edit')));
      break;

    case SAVED_UPDATED:
      backdrop_set_message(t('Updated newsletter category %name.', array('%name' => _simplenews_newsletter_name($category))));
      watchdog('simplenews', 'Updated newsletter category %name.', array('%name' => _simplenews_newsletter_name($category)), WATCHDOG_NOTICE, l(t('edit'), 'admin/config/services/simplenews/categories/' . $category->tid . '/edit'));
      break;
  }

  $form_state['values']['tid'] = $category->tid;
  $form_state['tid'] = $category->tid;
  $form_state['redirect'] = 'admin/config/services/simplenews';
}

/**
 * Menu callback: Delete newsletter category.
 *
 * @see simplenews_admin_category_delete_submit()
 */
function simplenews_admin_category_delete($form, &$form_state, $category) {
  $count = db_query('SELECT count(tid) as min_id FROM {simplenews_category}')->fetchField();
  if ($count == '1') {
    backdrop_set_message(t('At least one newsletter must exist. If you want to delete all newsletters, disable the SimpleNews module instead.'), 'error');
    backdrop_goto('admin/config/services/simplenews');
  }

  // Store some category values for submit handling.
  $form = array();
  $form['tid'] = array(
    '#type' => 'value',
    '#value' => $category->tid,
  );
  $form['name'] = array(
    '#type' => 'value',
    '#value' => _simplenews_newsletter_name($category),
  );

  $form['notice'] = array(
    '#markup' => '<p><strong>' . t('Note: All subscriptions associated with this newsletter will be lost.') . '</strong></p>',
  );

  return confirm_form($form, t('Are you sure you want to delete category %name?', array('%name' => _simplenews_newsletter_name($category))), 'admin/config/services/simplenews', t('This action cannot be undone.'), t('Delete'), t('Cancel')
  );
}

/**
 * Form submit callback for deleting a SimpleNews category.
 */
function simplenews_admin_category_delete_submit($form, &$form_state) {
  $tid = $form_state['values']['tid'];
  $name = $form_state['values']['name'];
  // Delete newsletter category and associated taxonomy term.
  // Subscriptions are deleted by simplenews_simplenews_category_delete()
  simplenews_category_delete($tid);
  taxonomy_term_delete($tid);
  backdrop_set_message(t('Newsletter category %name has been deleted.', array('%name' => $name)));

  $form_state['redirect'] = 'admin/config/services/simplenews';
  return;
}

/**
 * Menu callback: Mass subscribe to newsletters.
 *
 * @see simplenews_subscription_list_add_submit()
 *
 * @todo Add 32char description field as subscription source
 */
function simplenews_subscription_list_add($form, &$form_state) {
  global $language;

  $form['emails'] = array(
    '#type' => 'textarea',
    '#title' => t('Email addresses'),
    '#cols' => 60,
    '#rows' => 5,
    '#description' => t('Email addresses must be separated by comma, space or newline.'),
  );

  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Unsubscribe users by role'),
    '#description' => t('Select roles to unsubscribe all users assigned to them.'),
    '#tree' => TRUE,
  );

  foreach (user_roles(TRUE) as $role_name => $role_label) {
    $form['roles'][$role_name] = array(
      '#type' => 'checkbox',
      '#title' => check_plain($role_label),
    );
  }

  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Subscribe users by role'),
    '#description' => t('Select roles to subscribe all users assigned to them.'),
    '#tree' => TRUE,
  );

  foreach (user_roles(TRUE) as $role_name => $role_label) {
    $form['roles'][$role_name] = array(
      '#type' => 'checkbox',
      '#title' => check_plain($role_label),
    );
  }

  $form['newsletters'] = array(
    '#type' => 'fieldset',
    '#description' => t('Subscribe to'),
    '#tree' => TRUE,
  );

  foreach (simplenews_categories_load_multiple() as $list) {
    $form['newsletters'][$list->tid] = array(
      '#type' => 'checkbox',
      '#title' => check_plain(_simplenews_newsletter_name($list)),
      '#description' => _simplenews_newsletter_description($list),
    );
  }

  $form['resubscribe'] = array(
    '#type' => 'checkbox',
    '#title' => t('Force re-subscription'),
    '#description' => t('If checked, previously unsubscribed e-mail addresses will be re-subscribed. Consider that this might be against the will of your users.'),
  );

  // Include language selection when the site is multilingual.
  // Default value is the empty string which will result in receiving emails
  // in the site's default language.
  if (config_get('system.core', 'language_count') > 1) {
    $options[''] = t('Site default language');
    $languages = language_list('enabled');
    foreach ($languages as $langcode => $item) {
      $name = t($item->name);
      $options[$langcode] = $name . ($item->native != $name ? ' (' . $item->native . ')' : '');
    }
    $form['language'] = array(
      '#type' => 'radios',
      '#title' => t('Anonymous user preferred language'),
      '#default_value' => '',
      '#options' => $options,
      '#description' => t('New subscriptions will be subscribed with the selected preferred language. The language of existing subscribers is unchanged.'),
    );
  }
  else {
    $form['language'] = array(
      '#type' => 'value',
      '#value' => '',
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Subscribe'),
  );

  return $form;
}

/**
 * @todo
 */
function simplenews_subscription_list_add_submit($form, &$form_state) {
  $added = array();
  $invalid = array();
  $unsubscribed = array();
  $blocked = array();
  // Ensure blocked users are purged before processing new subscriptions.
  simplenews_unsubscribe_blocked_accounts();
  $checked_categories = array_keys(array_filter($form_state['values']['newsletters']));
  $langcode = $form_state['values']['language'];

  $emails = preg_split("/[\s,]+/", $form_state['values']['emails']);

  $selected_roles = !empty($form_state['values']['roles']) ? array_keys(array_filter($form_state['values']['roles'])) : array();
  if ($selected_roles) {
    $include_authenticated = in_array(BACKDROP_AUTHENTICATED_ROLE, $selected_roles, TRUE);
    $role_filter = array_diff($selected_roles, array(BACKDROP_AUTHENTICATED_ROLE));

    if ($role_filter) {
      $role_query = db_select('users', 'u');
      $role_query->join('users_roles', 'ur', 'u.uid = ur.uid');
      $role_query->fields('u', array('mail'));
      $role_query->condition('u.status', 1);
      $role_query->condition('u.mail', '', '<>');
      $role_query->condition('ur.role', $role_filter, 'IN');
      $emails = array_merge($emails, $role_query->execute()->fetchCol());
    }

    if ($include_authenticated) {
      // Authenticated role is implicit; pull all active users with an email.
      $auth_query = db_select('users', 'u')
        ->fields('u', array('mail'))
        ->condition('u.status', 1)
        ->condition('u.mail', '', '<>');
      $emails = array_merge($emails, $auth_query->execute()->fetchCol());
    }
  }

  // Normalize and deduplicate collected emails (manual + role-based).
  $emails = array_unique(array_filter(array_map('trim', $emails)));

  foreach ($emails as $email) {
    if (valid_email_address($email)) {
      $subscriber = simplenews_subscriber_load_by_mail($email);
      $unsubscribed_tids = array();
      if ($checked_categories) {
        $subscription_query = db_select('simplenews_subscription', 'ss');
        $subscription_query->join('simplenews_subscriber', 'sb', 'ss.snid = sb.snid');
        $subscription_query->fields('ss', array('tid'));
        $subscription_query->condition('sb.mail', $email);
        $subscription_query->condition('ss.tid', $checked_categories, 'IN');
        $subscription_query->condition('ss.status', SIMPLENEWS_SUBSCRIPTION_STATUS_UNSUBSCRIBED);
        $unsubscribed_tids = $subscription_query->execute()->fetchCol();
      }
      foreach (simplenews_categories_load_multiple($checked_categories) as $category) {
        // If there is a valid subscriber, check if there is a subscription for
        // the current category and if this subscription has the status
        // unsubscribed.
        $is_unsubscribed = (in_array($category->tid, $unsubscribed_tids, TRUE)) || ($subscriber && array_key_exists($category->tid, $subscriber->newsletter_subscription)
          && $subscriber->newsletter_subscription[$category->tid]->status == SIMPLENEWS_SUBSCRIPTION_STATUS_UNSUBSCRIBED);
        if (!$is_unsubscribed || $form_state['values']['resubscribe'] == TRUE) {
          if (simplenews_subscribe_user($email, $category->tid, FALSE, 'mass subscribe', $langcode, $form_state['values']['resubscribe'])) {
            $added[] = $email;
          }
          else {
            $blocked[] = $email;
          }
        }
        else {
          $unsubscribed[check_plain(_simplenews_newsletter_name($category))][] = $email;
        }
      }
    }
    else {
      $invalid[] = $email;
    }
  }
  if ($added) {
    $added = implode(", ", $added);
    backdrop_set_message(t('The following addresses were added or updated: %added.', array('%added' => $added)));

    $list_names = array();
    foreach (simplenews_categories_load_multiple($checked_categories) as $category) {
      $list_names[] = $category->name;
    }
    backdrop_set_message(t('The addresses were subscribed to the following newsletters: %newsletters.', array('%newsletters' => implode(', ', $list_names))));
  }
  else {
    form_set_error('mail', t("No addresses were added."));
  }
  if ($invalid) {
    $invalid = implode(", ", $invalid);
    backdrop_set_message(t('The following addresses were invalid: %invalid.', array('%invalid' => $invalid)), 'error');
  }

  if ($blocked) {
    $blocked = implode(", ", array_unique($blocked));
    backdrop_set_message(t('The following addresses are blocked users and were not subscribed: %blocked.', array('%blocked' => $blocked)), 'warning');
  }

  foreach ($unsubscribed as $name => $subscribers) {
    $subscribers = implode(", ", $subscribers);
    backdrop_set_message(t('The following addresses were skipped because they have previously unsubscribed from %name: %unsubscribed.', array('%name' => $name, '%unsubscribed' => $subscribers)), 'warning');
  }

  if (!empty($unsubscribed)) {
    backdrop_set_message(t("If you would like to re-subscribe them, use the 'Force re-subscription' option."), 'warning');
  }

  // Return to the parent page.
  $form_state['redirect'] = 'admin/people/simplenews';
}

/**
 * Menu callback: Export email address of subscriptions.
 *
 * @see simplenews_admin_export_after_build()
 */
function simplenews_subscription_list_export($form, &$form_state) {
  // Get sensible default values for the form elements in this form.
  $default['states'] = $_GET['states'] ?? array('active' => 'active');
  $default['subscribed'] = $_GET['subscribed'] ?? array('subscribed' => 'subscribed');
  $default['newsletters'] = $_GET['newsletters'] ?? array();

  $form['states'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Status'),
    '#options' => array(
      'active' => t('Active users'),
      'inactive' => t('Inactive users'),
    ),
    '#default_value' => $default['states'],
    '#description' => t('Subscriptions matching the selected states will be exported.'),
    '#required' => TRUE,
  );

  $form['subscribed'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Subscribed'),
    '#options' => array(
      'subscribed' => t('Subscribed to the newsletter'),
      'unconfirmed' => t('Unconfirmed to the newsletter'),
      'unsubscribed' => t('Unsubscribed from the newsletter'),
    ),
    '#default_value' => $default['subscribed'],
    '#description' => t('Subscriptions matching the selected subscription states will be exported.'),
    '#required' => TRUE,
  );

  $options = simplenews_category_list();
  $form['newsletters'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Newsletter'),
    '#options' => $options,
    '#default_value' => $default['newsletters'],
    '#description' => t('Subscriptions matching the selected newsletters will be exported.'),
    '#required' => TRUE,
  );

  // Get export results and display them in a text area. Only get the results
  // if the form is build after redirect, not after submit.
  if (isset($_GET['states']) && empty($form_state['input'])) {
    $form['emails'] = array(
      '#type' => 'textarea',
      '#title' => t('Export results'),
      '#cols' => 60,
      '#rows' => 5,
      '#value' => _simplenews_subscription_list_export_get_emails($_GET['states'], $_GET['subscribed'], $_GET['newsletters']),
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Export'),
  );
  return $form;
}

/**
 * @todo
 */
function simplenews_subscription_list_export_submit($form, &$form_state) {
  $form_values = $form_state['values'];

  // Get data for query string and redirect back to the current page.
  $options['query']['states'] = array_filter($form_values['states']);
  $options['query']['subscribed'] = array_filter($form_values['subscribed']);
  $options['query']['newsletters'] = array_keys(array_filter($form_values['newsletters']));
  $form_state['redirect'] = array('admin/people/simplenews/export', $options);
}

/**
 * Helper function to get comma separated list of emails to be exported.
 *
 * @param $states
 *   Array of subscriber states to filter on.
 * @param $subscribed
 *   Array of subscription states to filter on.
 * @param $newsletters
 *   Array of taxonomy ids to filter on.
 *
 * @return string
 *   Comma separated list of email addresses.
 */
function _simplenews_subscription_list_export_get_emails($states, $subscribed, $newsletters) {

  // Build conditions for active state, subscribed state and newsletter selection.
  if (isset($states['active'])) {
    $condition_active[] = 1;
  }
  if (isset($states['inactive'])) {
    $condition_active[] = 0;
  }
  if (isset($subscribed['subscribed'])) {
    $condition_subscribed[] = SIMPLENEWS_SUBSCRIPTION_STATUS_SUBSCRIBED;
  }
  if (isset($subscribed['unsubscribed'])) {
    $condition_subscribed[] = SIMPLENEWS_SUBSCRIPTION_STATUS_UNSUBSCRIBED;
  }
  if (isset($subscribed['unconfirmed'])) {
    $condition_subscribed[] = SIMPLENEWS_SUBSCRIPTION_STATUS_UNCONFIRMED;
  }

  // Get emails from the database.
  $query = db_select('simplenews_subscriber', 'ss');
  $query->innerJoin('simplenews_subscription', 'si', 'si.snid = ss.snid');
  $query->fields('ss', array('mail'))
    ->condition('ss.activated', $condition_active)
    ->condition('si.status', $condition_subscribed)
    ->condition('si.tid', $newsletters)
    ->distinct();
  $mails = $query->execute()->fetchCol(0);

  // Return comma separated array of emails or empty text.
  if ($mails) {
    return implode(", ", $mails);
  }
  return t('No addresses were found.');
}

/**
 * Menu callback: Mass subscribe to newsletters.
 *
 * @see simplenews_subscription_list_remove_submit()
 *
 * @todo Add 32char description field as unsubscription source
 */
function simplenews_subscription_list_remove($form, &$form_state) {
  $form['emails'] = array(
    '#type' => 'textarea',
    '#title' => t('Email addresses'),
    '#cols' => 60,
    '#rows' => 5,
    '#description' => t('Email addresses must be separated by comma, space or newline.'),
  );

  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Unsubscribe users by role'),
    '#description' => t('Select roles to unsubscribe all users assigned to them.'),
    '#tree' => TRUE,
  );

  foreach (user_roles(TRUE) as $role_name => $role_label) {
    $form['roles'][$role_name] = array(
      '#type' => 'checkbox',
      '#title' => check_plain($role_label),
    );
  }

  $form['newsletters'] = array(
    '#type' => 'fieldset',
    '#description' => t('Unsubscribe from'),
    '#tree' => TRUE,
  );

  foreach (simplenews_categories_load_multiple() as $category) {
    $form['newsletters'][$category->tid] = array(
      '#type' => 'checkbox',
      '#title' => check_plain(_simplenews_newsletter_name($category)),
      '#description' => _simplenews_newsletter_description($category),
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Unsubscribe'),
  );
  return $form;
}

/**
 * @todo
 */
function simplenews_subscription_list_remove_submit($form, &$form_state) {
  // Track which emails and which lists were actually unsubscribed.
  $removed = array();
  $removed_tids = array();
  $invalid = array();
  // Ensure blocked users are purged before processing.
  simplenews_unsubscribe_blocked_accounts();
  $checked_lists = array_keys(array_filter($form_state['values']['newsletters']));

  $emails = preg_split("/[\s,]+/", $form_state['values']['emails']);

  $selected_roles = !empty($form_state['values']['roles']) ? array_keys(array_filter($form_state['values']['roles'])) : array();
  if ($selected_roles) {
    $include_authenticated = in_array(BACKDROP_AUTHENTICATED_ROLE, $selected_roles, TRUE);
    $role_filter = array_diff($selected_roles, array(BACKDROP_AUTHENTICATED_ROLE));

    if ($role_filter) {
      $role_query = db_select('users', 'u');
      $role_query->join('users_roles', 'ur', 'u.uid = ur.uid');
      $role_query->fields('u', array('mail'));
      $role_query->condition('u.status', 1);
      $role_query->condition('u.mail', '', '<>');
      $role_query->condition('ur.role', $role_filter, 'IN');
      $emails = array_merge($emails, $role_query->execute()->fetchCol());
    }

    if ($include_authenticated) {
      $auth_query = db_select('users', 'u')
        ->fields('u', array('mail'))
        ->condition('u.status', 1)
        ->condition('u.mail', '', '<>');
      $emails = array_merge($emails, $auth_query->execute()->fetchCol());
    }
  }

  $emails = array_unique(array_filter(array_map('trim', $emails)));

  foreach ($emails as $email) {
    if (valid_email_address($email)) {
      foreach ($checked_lists as $tid) {
        if (simplenews_unsubscribe_user($email, $tid, FALSE, 'mass unsubscribe')) {
          $removed[] = $email;
          $removed_tids[] = $tid;
        }
      }
    }
    else {
      $invalid[] = $email;
    }
  }
  if ($removed) {
    // Collapse duplicate emails so the confirmation reads cleanly.
    $removed = implode(", ", array_unique($removed));
    backdrop_set_message(t('The following addresses were unsubscribed: %removed.', array('%removed' => $removed)));

    // Build human-readable list names; guard against missing category records
    // or empty translated names. Force a fresh load to avoid stale statics.
    $list_names = array();
    foreach (array_unique($removed_tids) as $tid) {
      $category = simplenews_category_load($tid, TRUE);
      if ($category) {
        $name = _simplenews_newsletter_name($category);
        if ($name === '' || $name === NULL) {
          // Try taxonomy term as a secondary source.
          if ($term = taxonomy_term_load($tid)) {
            $name = $term->name;
          }
        }
        if ($name === '' || $name === NULL) {
          $name = t('Newsletter @tid', array('@tid' => $tid));
        }
        $list_names[] = $name;
      }
      else {
        $list_names[] = t('Unknown (ID @tid)', array('@tid' => $tid));
      }
    }
    $list_names = array_unique($list_names);
    backdrop_set_message(t('The addresses were unsubscribed from the following newsletters: %newsletters.', array('%newsletters' => implode(', ', $list_names))));
  }
  else {
    form_set_error('mail', t("No addresses were removed (none were subscribed to the selected newsletters)."));
  }
  if (!empty($invalid[0])) {
    $invalid = implode(", ", $invalid);
    backdrop_set_message(t('The following addresses were invalid: %invalid.', array('%invalid' => $invalid)), 'error');
  }

  // Return to the parent page.
  $form_state['redirect'] = 'admin/people/simplenews';
}

/**
 * Menu callback: subscription administration.
 */
function simplenews_admin_subscription() {
  // @todo Fix the delete operation
  $form['filter'] = simplenews_subscription_filter_form();
  $form['#submit'][] = 'simplenews_subscription_filter_form_submit';
  $form['filter']['#theme'] = 'simplenews_filter_form';
  $form['admin'] = simplenews_subscription_list_form();

  return $form;
}

/**
 * Menu callback: subscription administration.
 *
 * @see simplenews_subscription_list_form_validate()
 * @see simplenews_subscription_list_form_submit()
 *
 * @todo Subscriber maintenance needs overhaul now we have more data available.
 */
function simplenews_subscription_list_form() {
  // Build an 'Update options' form.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $options = array();
  foreach (module_invoke_all('simplenews_subscription_operations') as $operation => $array) {
    $options[$operation] = $array['label'];
  }
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => 'activate',
  );
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#submit' => array('simplenews_subscription_list_form_submit'),
    '#validate' => array('simplenews_subscription_list_form_validate'),
  );

  // Table header. Used as tablesort default.
  $header = array(
    'mail' => array(
      'data' => t('Email'),
      'field' => 'sn.mail',
      'sort' => 'asc',
    ),
    'username' => array(
      'data' => t('Username'),
      'field' => 'u.name',
    ),
    'status' => array(
      'data' => t('Status'),
      'field' => 'sn.activated',
    ),
    'language' => array(
      'data' => t('Language'),
      'field' => 'sn.language',
    ),
    'operations' => array('data' => t('Operations')),
  );

  $query = db_select('simplenews_subscriber', 'sn')->extend('PagerDefault')->extend('TableSort');
  simplenews_build_subscription_filter_query($query);
  $query->leftJoin('users', 'u', 'sn.uid = u.uid');
  $query->innerJoin('simplenews_subscription', 'su', 'sn.snid = su.snid');
  $query->condition('su.status', SIMPLENEWS_SUBSCRIPTION_STATUS_SUBSCRIBED);
  $query->addField('u', 'name', 'name');
  $result = $query
    ->fields('sn', array('snid', 'activated', 'mail', 'uid', 'language'))
    ->limit(30)
    ->orderByHeader($header)
    ->execute();

  $options = array();
  $destination = backdrop_get_destination();

  foreach ($result as $subscriber) {
    $options[$subscriber->snid] = array(
      'mail' => check_plain($subscriber->mail),
      'username' => isset($subscriber->uid) ? l($subscriber->name, 'user/' . $subscriber->uid) : check_plain($subscriber->name),
      'status' => theme('simplenews_status', array('source' => 'activated', 'status' => $subscriber->activated)),
      'language' => check_plain($subscriber->language),
      'operations' => l(t('edit'), 'admin/people/simplenews/users/edit/' . $subscriber->snid, array(), $destination),
    );
  }

  $form['subscribers'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No subscribers available.'),
  );

  $form['pager'] = array(
    // Calling theme('pager') directly so that it the first call after the
    // pager query executed above.
    '#markup' => theme('pager'),
  );

  return $form;
}

/**
 * Implements hook_simplenews_subscription_operations().
 */
function simplenews_simplenews_subscription_operations() {
  $operations = array(
    'activate' => array(
      'label' => t('Activate'),
      'callback' => 'simplenews_subscription_activate',
      'callback arguments' => array(SIMPLENEWS_SUBSCRIPTION_ACTIVE),
    ),
    'inactivate' => array(
      'label' => t('Inactivate'),
      'callback' => 'simplenews_subscription_activate',
      'callback arguments' => array(SIMPLENEWS_SUBSCRIPTION_INACTIVE),
    ),
    'delete' => array(
      'label' => t('Delete'),
      'callback' => 'simplenews_subscription_delete_multiple',
    ),
  );
  return $operations;
}

/**
 * @todo
 */
function simplenews_subscription_list_form_validate($form, &$form_state) {
  if (isset($form_state['values']['operation'])) {
    $snids = array_keys(array_filter($form_state['values']['subscribers']));
    if (empty($snids)) {
      form_set_error('', t('No items selected.'));
    }
  }
}

/**
 * @todo
 */
function simplenews_subscription_list_form_submit($form, &$form_state) {
  // Call operation functions as defined in hook_simplenews_subscription_operations().
  $operations = module_invoke_all('simplenews_subscription_operations');
  $operation = $operations[$form_state['values']['operation']];
  // Filter out unchecked subscribers.
  $snids = array_filter($form_state['values']['subscribers']);
  if ($function = $operation['callback']) {
    // Add in callback arguments if present.
    if (isset($operation['callback arguments'])) {
      $args = array_merge(array($snids), $operation['callback arguments']);
    }
    else {
      $args = array($snids);
    }
    call_user_func_array($function, $args);

    backdrop_set_message(t('The update has been performed.'));
  }
  else {
    // We need to rebuild the form to go to a second step. For example, to
    // show the confirmation form for the deletion of nodes.
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Callback function to (de-)activate subscriptions.
 *
 * @param $snids
 *   Array of snid's to be activated.
 * @param $status
 *   Status of the subscription (0, 1).
 */
function simplenews_subscription_activate($snids, $status) {
  foreach ($snids as $snid) {
    $subscriber = simplenews_subscriber_load($snid);
    $subscriber->activated = $status;
    simplenews_subscriber_save($subscriber);
  }
}

/**
 * Callback function to delete subscriptions.
 *
 * @param $snids
 *   Array of snid's to be deleted.
 */
function simplenews_subscription_delete_multiple($snids = array()) {
  foreach ($snids as $snid) {
    // Delete Subscription.
    simplenews_subscription_delete(array('snid' => $snid));
    // Delete subscriber.
    $subscriber = simplenews_subscriber_load($snid);
    simplenews_subscriber_delete($subscriber);
  }
}

/**
 * Menu callback: SimpleNews admin settings - Newsletter.
 */
function simplenews_admin_settings_newsletter($form, &$form_state) {
  $config = config('simplenews.settings');
  $form = array();
  $form['#config'] = 'simplenews.settings';

  $form['markup'] = array(
    '#markup' => '<ul class="action-links"><li><a href="/admin/people/simplenews">Manage newsletter subscriptions</a></li></ul>',
  );

  $form['simplenews_default_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default newsletter options'),
    '#collapsible' => FALSE,
    '#description' => t('These options will be the defaults for new newsletters, but can be overridden in the newsletter editing form.'),
  );
  $links = array(
    '!mime_mail_url' => 'https://backdropcms.org/project/mimemail',
  );
  $description = t('Default newsletter format. Install the <a href="!mime_mail_url">Mime Mail</a> module to send newsletters in HTML format.', $links);
  $form['simplenews_default_options']['format'] = array(
    '#type' => 'select',
    '#title' => t('Format'),
    '#options' => simplenews_format_options(),
    '#description' => $description,
    '#default_value' => $config->get('format'),
  );
  // @todo Do we need these master defaults for 'priority' and 'receipt'?
  $form['simplenews_default_options']['priority'] = array(
    '#type' => 'select',
    '#title' => t('Priority'),
    '#options' => simplenews_get_priority(),
    '#description' => t('Note that email priority is ignored by a lot of email programs.'),
    '#default_value' => $config->get('priority'),
  );
  $form['simplenews_default_options']['receipt'] = array(
    '#type' => 'checkbox',
    '#title' => t('Request receipt'),
    '#default_value' => $config->get('receipt'),
    '#description' => t('Request a Read Receipt from your newsletters. A lot of email programs ignore these so it is not a definitive indication of how many people have read your newsletter.'),
  );
  $form['simplenews_default_options']['send'] = array(
    '#type' => 'radios',
    '#title' => t('Default send action'),
    '#options' => array(
      SIMPLENEWS_COMMAND_SEND_TEST => t('Send one test newsletter to the test address'),
      SIMPLENEWS_COMMAND_SEND_NOW => t('Send newsletter'),
    ),
    '#default_value' => $config->get('send'),
  );
  $form['simplenews_default_options']['send_progress_refresh'] = array(
    '#type' => 'number',
    '#title' => t('Delivery progress auto-refresh interval'),
    '#description' => t('Seconds between automatic refreshes of the Send tab progress while messages are queued. Minimum 3 seconds. Set this close to your site cron interval to balance load and freshness.'),
    '#default_value' => $config->get('send_progress_refresh') ?: 60,
    '#min' => 3,
    '#max' => 3600,
    '#step' => 1,
  );
  $form['simplenews_test_address'] = array(
    '#type' => 'fieldset',
    '#title' => t('Test addresses'),
    '#collapsible' => FALSE,
    '#description' => t('Supply a comma-separated list of email addresses to be used as test addresses. The override function allows to override these addresses in the newsletter editing form.'),
  );
  $form['simplenews_test_address']['test_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Email address'),
    '#size' => 60,
    '#maxlength' => 128,
    '#default_value' => $config->get('test_address'),
  );
  $form['simplenews_test_address']['test_address_override'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow test address override'),
    '#default_value' => $config->get('test_address_override'),
  );
  $form['simplenews_sender_info'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sender information'),
    '#collapsible' => FALSE,
    '#description' => t('Default sender address that will only be used for confirmation emails. You can specify sender information for each newsletter separately on the newsletter\'s settings page.'),
  );
  $form['simplenews_sender_info']['from_name'] = array(
    '#type' => 'textfield',
    '#title' => t('From name'),
    '#size' => 60,
    '#maxlength' => 128,
    '#default_value' => $config->get('from_name'),
  );
  $form['simplenews_sender_info']['from_address'] = array(
    '#type' => 'textfield',
    '#title' => t('From email address'),
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => TRUE,
    '#default_value' => $config->get('from_address'),
  );
  return system_settings_form($form);
}

/**
 * Validate form.
 */
function simplenews_admin_settings_newsletter_validate($form, &$form_state) {
  if (!valid_email_address($form_state['values']['from_address'])) {
    form_set_error('from_address', t("The sender's email address you supplied is not valid."));
  }

  $refresh_seconds = (int) $form_state['values']['send_progress_refresh'];
  if ($refresh_seconds < 3) {
    form_set_error('send_progress_refresh', t('Auto-refresh interval must be at least 3 seconds.'));
  }
}

/**
 * Menu callback: SimpleNews admin settings - Email.
 */
function simplenews_admin_settings_mail($form, &$form_state) {
  $config = config('simplenews.settings');
  $form = array();
  $form['#config'] = 'simplenews.settings';

  $form['markup'] = array(
    '#markup' => '<ul class="action-links"><li><a href="/admin/people/simplenews">Manage newsletter subscriptions</a></li></ul>',
  );

  $form['excluded_domains'] = array(
    '#type' => 'textarea',
    '#title' => t('Exclude e-mail servers'),
    '#default_value' => $config->get('excluded_domains'),
    '#description' => t('Some email servers have strict anti-spam policies and may reject your newsletters for various reasons, such as improperly configured SPF, DMARC, or DKIM records for your domain. To prevent repeated delivery attempts and the risk of being permanently blocklisted, it’s recommended to temporarily exclude users with email addresses hosted by these servers until the issue is fully addressed. Please enter the domain names, separating them with commas (e.g., att.com, yahoo.com, gmail.com).'),
  );

  $form['simplenews_mail_backend']['use_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use cron to send newsletters'),
    '#default_value' => $config->get('use_cron'),
    '#description' => t('When checked cron will be used to send newsletters (recommended). Test newsletters and confirmation emails will be sent immediately. Leave unchecked for testing purposes.'),
  );

  $sources = simplenews_get_source_caches();
  $sources_labels = array();
  $sources_descriptions = '';
  foreach ($sources as $name => $source) {
    $sources_labels[$name] = $source['label'];
    $sources_descriptions .= t('<strong>@label</strong>: @description <br />', array('@label' => $source['label'], '@description' => $source['description']));
  }

  $form['simplenews_mail_backend']['source_cache'] = array(
    '#type' => 'select',
    '#title' => t('Cache'),
    '#description' => t('Choosing a different cache implementation allows for a different behavior during sending mails.') . '<br /><br />' . $sources_descriptions,
    '#options' => $sources_labels,
    '#default_value' => $config->get('source_cache'),
  );

  $throttle = backdrop_map_assoc(array(1, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000));
  $throttle[SIMPLENEWS_UNLIMITED] = t('Unlimited');
  if (function_exists('getrusage')) {
    $description_extra = '<br />' . t('Cron execution must not exceed the PHP maximum execution time of %max seconds. You find the time spend to send emails in the <a href="/admin/reports/dblog">Recent log entries</a>.', array('%max' => ini_get('max_execution_time')));
  }
  else {
    $description_extra = '<br />' . t('Cron execution must not exceed the PHP maximum execution time of %max seconds.', array('%max' => ini_get('max_execution_time')));
  }
  $form['simplenews_mail_backend']['throttle'] = array(
    '#type' => 'select',
    '#title' => t('Cron throttle'),
    '#options' => $throttle,
    '#default_value' => $config->get('throttle'),
    '#description' => t('Sets the numbers of newsletters sent per cron run. Failure to send will also be counted.') . $description_extra,
  );
  $form['simplenews_mail_backend']['spool_expire'] = array(
    '#type' => 'select',
    '#title' => t('Mail spool expiration'),
    '#options' => array(
      0 => t('Immediate'),
      1 => format_plural(1, '1 day', '@count days'),
      7 => format_plural(1, '1 week', '@count weeks'),
      14 => format_plural(2, '1 week', '@count weeks'),
    ),
    '#default_value' => $config->get('spool_expire'),
    '#description' => t('Newsletter mails are spooled. How long must messages be retained in the spool after successful sending. Keeping the message in the spool allows mail statistics (which is not yet implemented). If cron is not used, immediate expiration is advised.'),
  );
  $form['simplenews_mail_backend']['debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('Log emails'),
    '#default_value' => $config->get('debug'),
    '#description' => t('Logs outgoing SimpleNews emails at Debug severity. Logged means “handed to PHP mail()”, not delivered. Enable Debug severity at !logging to view entries.', array('!logging' => l(t('Configuration » Development » Logging and errors'), 'admin/config/development/logging'))),
  );
  return system_settings_form($form);
}

/**
 * Menu callback: SimpleNews admin settings - Subscription.
 */
function simplenews_admin_settings_subscription($form, &$form_state) {
  $config = config('simplenews.settings');
  $form = array();
  $form['#config'] = 'simplenews.settings';

  $form['markup'] = array(
    '#markup' => '<ul class="action-links"><li><a href="/admin/people/simplenews">Manage newsletter subscriptions</a></li></ul>',
  );

  $form['account'] = array(
    '#type' => 'fieldset',
    '#title' => t('User account'),
    '#collapsible' => FALSE,
  );
  $form['account']['sync_account'] = array(
    '#type' => 'checkbox',
    '#title' => t('Synchronize with account'),
    '#default_value' => $config->get('sync_account'),
    '#description' => t('When enabled, newsletter subscriptions will be synchronized with site user accounts. Subscriptions matching deleted accounts will be removed, and subscriptions will be deactivated or activated based on whether the associated account is blocked or unblocked. If disabled, subscriptions will remain unchanged regardless of account status.'),
  );

  $form['subscription_mail'] = array(
    '#type' => 'fieldset',
    '#title' => t('Confirmation emails'),
    '#collapsible' => FALSE,
  );
  $form['subscription_mail']['use_combined'] = array(
    '#type' => 'select',
    '#title' => t('Use combined confirmation mails'),
    '#options' => array(
      'multiple' => t('For multiple changes'),
      'always' => t('Always'),
      'never' => t('Never'),
    ),
    '#description' => t('Combined confirmation mails allow subscribers to confirm multiple newsletter changes with single mail.'),
    '#default_value' => $config->get('use_combined'),
  );

  if (module_exists('token')) {
    $form['subscription_mail']['token_help'] = array(
      '#title' => t('Replacement patterns'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $form['subscription_mail']['token_help']['browser'] = array(
      '#theme' => 'token_tree',
      '#token_types' => array('simplenews-category', 'simplenews-subscriber'),
    );
  }

  $form['subscription_mail']['single'] = array(
    '#type' => 'fieldset',
    '#title' => t('Single confirmation mails'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#states' => array(
      'invisible' => array(
        ':input[name="use_combined"]' => array(
          'value' => 'always',
        ),
      ),
    ),
  );

  $form['subscription_mail']['single']['confirm_subscribe_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#default_value' => simplenews_subscription_confirmation_text('subscribe_subject'),
    '#maxlength' => 180,
  );
  $form['subscription_mail']['single']['confirm_subscribe_unsubscribed'] = array(
    '#type' => 'textarea',
    '#title' => t('Body text of subscribe email'),
    '#default_value' => simplenews_subscription_confirmation_text('subscribe_unsubscribed'),
    '#rows' => 5,
  );
  $form['subscription_mail']['single']['confirm_subscribe_subscribed'] = array(
    '#type' => 'textarea',
    '#title' => t('Body text for already subscribed visitor'),
    '#default_value' => simplenews_subscription_confirmation_text('subscribe_subscribed'),
    '#rows' => 5,
  );
  $form['subscription_mail']['single']['confirm_unsubscribe_subscribed'] = array(
    '#type' => 'textarea',
    '#title' => t('Body text of unsubscribe email'),
    '#default_value' => simplenews_subscription_confirmation_text('unsubscribe_subscribed'),
    '#rows' => 5,
  );
  $form['subscription_mail']['single']['confirm_unsubscribe_unsubscribed'] = array(
    '#type' => 'textarea',
    '#title' => t('Body text for not yet subscribed visitor'),
    '#default_value' => simplenews_subscription_confirmation_text('unsubscribe_unsubscribed'),
    '#rows' => 5,
  );

  $form['subscription_mail']['combined'] = array(
    '#type' => 'fieldset',
    '#title' => t('Combined confirmation mails'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#states' => array(
      'invisible' => array(
        ':input[name="use_combined"]' => array(
          'value' => 'never',
        ),
      ),
    ),
  );

  $form['subscription_mail']['combined']['confirm_combined_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject text for combined confirmation mail'),
    '#default_value' => simplenews_subscription_confirmation_text('combined_subject'),
  );

  $form['subscription_mail']['combined']['confirm_combined_body'] = array(
    '#type' => 'textarea',
    '#title' => t('Body text for combined confirmation mail'),
    '#default_value' => simplenews_subscription_confirmation_text('combined_body'),
    '#rows' => 5,
  );

  $form['subscription_mail']['combined']['confirm_combined_body_unchanged'] = array(
    '#type' => 'textarea',
    '#title' => t('Body text for unchanged combined confirmation mail'),
    '#default_value' => simplenews_subscription_confirmation_text('combined_body_unchanged'),
    '#rows' => 5,
    '#description' => t('This body is used when there are no change requests which have no effect, e.g trying to subscribe when already being subscribed to a category.'),
  );

  $form['subscription_mail']['combined']['confirm_combined_line_subscribe_unsubscribed'] = array(
    '#type' => 'textfield',
    '#title' => t('Change text for a new subscription'),
    '#default_value' => simplenews_subscription_confirmation_text('combined_line_subscribe_unsubscribed'),
  );

  $form['subscription_mail']['combined']['confirm_combined_line_subscribe_subscribed'] = array(
    '#type' => 'textfield',
    '#title' => t('Change text when already subscribed'),
    '#default_value' => simplenews_subscription_confirmation_text('combined_line_subscribe_subscribed'),
  );

  $form['subscription_mail']['combined']['confirm_combined_line_unsubscribe_subscribed'] = array(
    '#type' => 'textfield',
    '#title' => t('Change text for an unsubscription'),
    '#default_value' => simplenews_subscription_confirmation_text('combined_line_unsubscribe_subscribed'),
  );

  $form['subscription_mail']['combined']['confirm_combined_line_unsubscribe_unsubscribed'] = array(
    '#type' => 'textfield',
    '#title' => t('Change text when already unsubscribed'),
    '#default_value' => simplenews_subscription_confirmation_text('combined_line_unsubscribe_unsubscribed'),
  );

  $form['confirm_pages'] = array(
    '#type' => 'fieldset',
    '#title' => t('Confirmation pages'),
    '#collapsible' => FALSE,
  );
  $form['confirm_pages']['confirm_subscribe_page'] = array(
    '#type' => 'textfield',
    '#title' => t('Subscribe confirmation'),
    '#description' => t('Backdrop path or URL of the destination page where after the subscription is confirmed (e.g., node/123). Leave empty to go to the front page.'),
    '#default_value' => $config->get('confirm_subscribe_page'),
  );
  $form['confirm_pages']['confirm_unsubscribe_page'] = array(
    '#type' => 'textfield',
    '#title' => t('Unsubscribe confirmation'),
    '#description' => t('Backdrop path or URL of the destination page when the subscription removal is confirmed (e.g., node/123). Leave empty to go to the front page.'),
    '#default_value' => $config->get('confirm_unsubscribe_page'),
  );

  $form['notify_admins'] = [
    '#type' => 'fieldset',
    '#title' => t('Notify administrators'),
    '#collapsible' => FALSE,
    '#description' => t('All users with the selected roles will receive notifications whenever someone subscribes to or unsubscribes from newsletters.'),
  ];

  $role_configs = config_get_names_with_prefix('user.role.');

  foreach ($role_configs as $role_config_name) {
    $role = config($role_config_name);
    $role_name = $role->get('name');
    $role_label = $role->get('label');
    $permissions = $role->get('permissions');

    if (in_array('administer newsletters', $permissions)) {
      $config_key = 'notify_' . $role_name . '_role_members';

      $form['notify_admins'][$config_key] = [
        '#type' => 'checkbox',
        '#title' => t($role_label),
        '#default_value' => $config->get($config_key),
      ];
    }
  }

  return system_settings_form($form);
}

/**
 * Generate subscription filters.
 */
function simplenews_subscription_filters() {
  // Newsletter filter.
  $filters['list'] = array(
    'title' => t('Subscribed to'),
    'options' => array(
      'all' => t('All newsletters'),
    ),
  );
  foreach (simplenews_categories_load_multiple() as $list) {
    $filters['list']['options']['tid-' . $list->tid] = _simplenews_newsletter_name($list);
  }

  // Email filter.
  $filters['email'] = array(
    'title' => t('Email address'),
  );

  return $filters;
}

/**
 * Return form for subscription filters.
 *
 * @see simplenews_subscription_filter_form_submit()
 */
function simplenews_subscription_filter_form() {
  // Current filter selections in $session var; stored at form submission
  // Example: array('list' => 'all', 'email' => 'hotmail')
  $session = $_SESSION['simplenews_subscriptions_filter'] ?? '';
  $session = is_array($session) ? $session : _simplenews_subscription_filter_default();
  $filters = simplenews_subscription_filters();

  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Show only subscription which'),
    '#collapsible' => FALSE,
  );

  // Filter values are default.
  $form['filters']['list'] = array(
    '#type' => 'select',
    '#title' => check_plain($filters['list']['title']),
    '#options' => $filters['list']['options'],
    '#default_value' => $session['list'],
  );
  $form['filters']['email'] = array(
    '#type' => 'textfield',
    '#title' => check_plain($filters['email']['title']),
    '#default_value' => $session['email'],
  );
  $form['filters']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#prefix' => '<span class="spacer" />',
  );
  // Add Reset button if filter is in use.
  if ($session != _simplenews_subscription_filter_default()) {
    $form['filters']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
    );
  }
  $form['#attached']['css'][] = backdrop_get_path('module', 'simplenews') . '/simplenews.css';

  return $form;
}

/**
 * Helper function: returns subscription filter default settings.
 */
function _simplenews_subscription_filter_default() {
  return array(
    'list' => 'all',
    'email' => '',
  );
}

/**
 * @todo
 */
function simplenews_subscription_filter_form_submit($form, &$form_state) {
  switch ($form_state['values']['op']) {
    case t('Filter'):
      $_SESSION['simplenews_subscriptions_filter'] = array(
        'list' => $form_state['values']['list'],
        'email' => $form_state['values']['email'],
      );
      break;

    case t('Reset'):
      $_SESSION['simplenews_subscriptions_filter'] = _simplenews_subscription_filter_default();
      break;
  }
}

/**
 * Apply filters for subscription filters based on session.
 *
 * @param $query
 *   A SelectQuery to which the filters should be applied.
 */
function simplenews_build_subscription_filter_query(SelectQueryInterface $query) {
  if (isset($_SESSION['simplenews_subscriptions_filter'])) {
    foreach ($_SESSION['simplenews_subscriptions_filter'] as $key => $value) {
      switch ($key) {
        case 'list':
          if ($value != 'all') {
            [$key, $value] = explode('-', $value, 2);
            $query->condition('su.' . $key, $value);
          }
          break;

        case 'email':
          if (!empty($value)) {
            $query->condition('sn.mail', '%' . $value . '%', 'LIKE');
          }
          break;
      }
    }
  }
}

/**
 * Count number of subscribers per newsletter list.
 *
 * @return number of subscribers.
 */
function simplenews_count_subscriptions($tid) {
  $subscription_count = &backdrop_static(__FUNCTION__);

  if (isset($subscription_count[$tid])) {
    return $subscription_count[$tid];
  }
  $query = db_select('simplenews_subscription', 'ss');
  $query->leftJoin('simplenews_subscriber', 'sn', 'sn.snid = ss.snid');
  $query->condition('tid', $tid)
    ->condition('sn.activated', 1)
    ->condition('status', SIMPLENEWS_SUBSCRIPTION_STATUS_SUBSCRIBED);
  $subscription_count[$tid] = $query->countQuery()->execute()->fetchField();
  return $subscription_count[$tid];
}

/**
 * Return a status image.
 *
 * @param $variables
 *   An associative array containing:
 *   source: Source which status will be displayed ('published', 'activated', 'sent')
 *   status: Status of the source (0 or 1)
 *
 * @return string
 *   HTML string containing an image tag.
 *
 * @ingroup theming
 */
function theme_simplenews_status($variables) {
  $source = $variables['source'];
  $status = $variables['status'];
  switch ($source) {
    case 'published':
      $images = array(
        0 => 'sn-saved.png',
        1 => 'sn-sent.png',
      );
      $title = array(
        0 => t('Not published'),
        1 => t('Published'),
      );
      break;

    case 'activated':
      $images = array(
        0 => 'sn-saved.png',
        1 => 'sn-sent.png',
      );
      $title = array(
        0 => t('Inactive: no newsletters will be sent'),
        1 => t('Active: user will receive newsletters'),
      );
      break;

    case 'sent':
      $images = array(
        SIMPLENEWS_STATUS_SEND_PENDING => 'sn-cron.png',
        SIMPLENEWS_STATUS_SEND_READY => 'sn-sent.png',
      );
      $title = array(
        SIMPLENEWS_STATUS_SEND_NOT => t('Not yet sent'),
        SIMPLENEWS_STATUS_SEND_PENDING => t('Currently sending by cron'),
        SIMPLENEWS_STATUS_SEND_READY => t('Sent'),
        SIMPLENEWS_STATUS_SEND_PUBLISH => t('Send on publish'),
      );
      break;
  }

  // Build the output.
  if (isset($images[$status])) {
    $img_vars = array(
      'path' => backdrop_get_path('module', 'simplenews') . '/' . $images[$status],
      'alt' => $title[$status],
      'title' => $title[$status],
      'getsize' => TRUE,
    );
    $output = theme('image', $img_vars);
  }
  else {
    $output = check_plain($title[$status]);
  }
  return $output;
}

/**
 * Theme SimpleNews issues and subscriptions filter form.
 *
 * @ingroup theming
 */
function theme_simplenews_filter_form($variables) {
  $form = $variables['form'];

  $output = '<div id="simplenews-admin-filter">';
  $output .= backdrop_render($form['filters']);
  $output .= '</div>';
  $output .= backdrop_render_children($form);
  return $output;
}

/**
 * Theme SimpleNews subscriptions administration filter form.
 *
 * @ingroup theming
 */
function theme_simplenews_subscription_filter_form($variables) {
  // @todo Should this theme simplenews_subscription_filter_form be declared in hook_theme()?
  $form = $variables['form'];

  $output = '<div id="simplenews-subscription-filter">';
  $output .= backdrop_render($form['filters']);
  $output .= '</div>';
  $output .= backdrop_render_children($form);
  return $output;
}

/**
 * Function to provide the various SimpleNews mail priorities for newsletter categories.
 */
function simplenews_get_priority() {
  return array(
    SIMPLENEWS_PRIORITY_NONE => t('none'),
    SIMPLENEWS_PRIORITY_HIGHEST => t('highest'),
    SIMPLENEWS_PRIORITY_HIGH => t('high'),
    SIMPLENEWS_PRIORITY_NORMAL => t('normal'),
    SIMPLENEWS_PRIORITY_LOW => t('low'),
    SIMPLENEWS_PRIORITY_LOWEST => t('lowest'),
  );
}

/**
 * Menu callback; Newsletter tab page.
 */
function simplenews_node_tab_page($node) {
  if (!empty($node->simplenews->tid)) {
    backdrop_set_title(t('<em>Newsletter</em> @title', array('@title' => $node->title)), PASS_THROUGH);
    return backdrop_get_form('simplenews_node_tab_send_form', $node);
  }
  else {
    backdrop_set_message(t('No newsletter has been selected for this page. Please choose one to proceed.'), 'warning');
    backdrop_goto('/node/' . $node->nid . '/edit', array('query' => array('destination' => '/node/' . $node->nid . '/simplenews')));
  }
}

/**
 * @todo
 */
function simplenews_node_tab_send_form($form, &$form_state, $node) {

  $config = config('simplenews.settings');
  $simplenews_term = $config->get('category_field');

  // Make sure a term is loaded and it is of newsletter category.
  $simplenews_term = taxonomy_term_load($node->{$simplenews_term}[LANGUAGE_NONE][0]['tid']);
  if (empty($simplenews_term)) {
    backdrop_set_message(t('No newsletters have been selected for this page. Please choose one to proceed.'), 'error');
    backdrop_goto('/node/' . $node->nid . '/edit');
  }
  elseif ($simplenews_term->vocabulary != 'newsletter') {
    // Only allow terms from the newsletter vocabulary on the send tab.
    backdrop_set_message(t('Category selected for this page is not a newsletter category. Please choose a newsletter category.'), 'error');
    backdrop_goto('/node/' . $node->nid . '/edit');
  }

  // First check if there already is a loaded SimpleNews object.
  if (!empty($node->simplenews)) {
    $simplenews_values = $node->simplenews;
  }
  // If not, try to load it based on the node id.
  elseif ($loaded = simplenews_newsletter_load($node->nid)) {
    $simplenews_values = $loaded;
  }
  // If that fails too, fall back to the defaults.
  else {
    $simplenews_values = (object) _simplenews_get_node_form_defaults($node);
  }

  $form = array();
  // We will need the node.
  $form['nid'] = array(
    '#type' => 'value',
    '#value' => $node->nid,
  );

  // @todo delete this fieldset?
  $form['simplenews'] = array(
    '#type' => 'fieldset',
    '#title' => t('Send newsletter'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );

  // Show send progress inside the Send newsletter fieldset so admins can see
  // how many have gone out vs remain.
  module_load_include('inc', 'simplenews', 'includes/simplenews.mail');
  // Reload newsletter to avoid stale counts after sending.
  $fresh_newsletter = simplenews_newsletter_load($node->nid, TRUE);
  $sent_from_newsletter = !empty($fresh_newsletter->sent_subscriber_count) ? $fresh_newsletter->sent_subscriber_count : 0;

  // Also check the per-category status table to pick up the latest persisted
  // count, even if the main newsletter row hasn’t been updated yet.
  $sent_from_status = 0;
  $tid_for_status = $fresh_newsletter->tid ?? (!empty($simplenews_values->tid) ? $simplenews_values->tid : NULL);
  if ($tid_for_status) {
    if ($status_row = simplenews_newsletter_status_load($node->nid, $tid_for_status)) {
      $sent_from_status = $status_row->sent_subscriber_count ?? 0;
    }
  }

  // Also look at the spool for DONE (or expired IN_PROGRESS) messages to show
  // in-flight progress before persistence catches up.
  $sent_count_query = db_select('simplenews_mail_spool')
    ->condition('nid', $node->nid)
    ->condition(db_or()
      ->condition('status', SIMPLENEWS_SPOOL_DONE)
      ->condition(db_and()
        ->condition('status', SIMPLENEWS_SPOOL_IN_PROGRESS)
        ->condition('timestamp', simplenews_get_expiration_time(), '<='))
    )
    ->countQuery();
  $sent_from_spool = (int) $sent_count_query->execute()->fetchField();

  $sent_count = max($sent_from_newsletter, $sent_from_status, $sent_from_spool);
  $pending_count = simplenews_count_spool(array('nid' => $node->nid));
  $total_count = $sent_count + $pending_count;
  $percent = $total_count ? round(($sent_count / $total_count) * 100) : 0;

  // Friendly status label + color.
  if ($pending_count > 0 && $sent_count > 0) {
    $status_label = t('Sending');
    $status_bg = '#f0ad4e';
    $status_fg = '#2b2b2b';
  }
  elseif ($pending_count > 0 && $sent_count == 0) {
    $status_label = t('Ready to send');
    $status_bg = '#5bc0de';
    $status_fg = '#0f2a33';
  }
  elseif ($pending_count == 0 && $sent_count > 0) {
    $status_label = t('All sent');
    $status_bg = '#5cb85c';
    $status_fg = '#0f2910';
  }
  else {
    $status_label = t('Not started');
    $status_bg = '#d9d9d9';
    $status_fg = '#333';
  }

  $bar_outer_style = 'margin-top:6px;height:12px;border-radius:6px;background:#e5e5e5;overflow:hidden;';
  $bar_inner_style = 'height:12px;border-radius:6px;background:#5cb85c;width:' . $percent . '%;transition:width 0.3s;';
  $status_style = 'display:inline-block;padding:2px 10px;border-radius:999px;font-size:12px;font-weight:600;background:' . $status_bg . ';color:' . $status_fg . ';';

  $refresh_seconds = (int) $config->get('send_progress_refresh');
  if ($refresh_seconds < 3) {
    $refresh_seconds = 60;
  }

  $progress_markup = '<div class="sn-progress-wrapper" data-pending="' . $pending_count . '">' .
    '<div class="sn-progress-summary">' .
    t('@sent of @total delivered', array('@sent' => $sent_count, '@total' => $total_count)) .
    '</div>' .
    '<div class="sn-progress-bar" style="' . $bar_outer_style . '"><div style="' . $bar_inner_style . '"></div></div>' .
    '<div class="sn-progress-queue">' . t('Queued to send: @pending', array('@pending' => $pending_count)) . '</div>' .
    '<div class="sn-progress-status" style="margin-top:6px;">' . t('Status: !status', array('!status' => '<span style="' . $status_style . '">' . check_plain($status_label) . '</span>')) . '</div>' .
    '<div class="sn-progress-note" style="margin-top:4px;color:#666;' . ($pending_count ? '' : 'display:none;') . '">' . t('Updating automatically. Next refresh in @sec seconds.', array('@sec' => $refresh_seconds)) . '</div>' .
    '</div>';

  $form['simplenews']['send_progress'] = array(
    '#type' => 'item',
    '#title' => t('Delivery progress'),
    '#markup' => $progress_markup,
    '#weight' => 50,
    '#suffix' => '<hr />',
  );

  // Auto-refresh the tab while messages remain queued; stop once all sent.
  $auto_refresh = ($pending_count > 0);
  $form['#attached']['js'][] = array(
    'type' => 'inline',
    'data' => '(function ($) { Backdrop.behaviors.simplenewsSendProgress = { attach: function (context) { $("#simplenews-node-tab-send-form", context).once("simplenews-send-refresh").each(function () { var hasPending = ' . ($auto_refresh ? 'true' : 'false') . '; var refreshSec = ' . $refresh_seconds . '; var $form = $(this); var $wrapper = $(".sn-progress-wrapper", this); var $note = $(".sn-progress-note", this); var tick; var remaining = refreshSec; function setNote(sec) { if ($note.length) { $note.show().text(Backdrop.t("Updating automatically. Next refresh in @sec seconds.", {"@sec": sec})); } } function refreshProgress() { $.get(window.location.href, function (response) { var $resp = $(response); var $newWrapper = $resp.filter(".sn-progress-wrapper"); if (!$newWrapper.length) { $newWrapper = $resp.find(".sn-progress-wrapper"); } if (!$newWrapper.length) { return; } $wrapper.replaceWith($newWrapper); $wrapper = $newWrapper; $note = $(".sn-progress-note", $wrapper); var pendingAttr = parseInt($wrapper.data("pending"), 10); hasPending = pendingAttr > 0; if (!hasPending) { if ($note.length) { $note.hide(); } if (tick) { clearInterval(tick); } return; } remaining = refreshSec; setNote(remaining); }); } if (hasPending) { setNote(remaining); tick = setInterval(function () { remaining--; if (remaining <= 0) { refreshProgress(); } else { setNote(remaining); } }, 1000); $form.data("simplenewsSendRefreshTimer", tick); } else { if ($note.length) { $note.hide(); } tick = $form.data("simplenewsSendRefreshTimer"); if (tick) { clearInterval(tick); } } }); } }; })(jQuery);',
  );

  // Translations of newsletters don't have the 'send' option. Only the
  // translation source (and non translated) newsletters will get these options.
  if (module_exists('translation') && translation_supported_type($node->type) && (isset($node->translate) && ($node->tnid > 0) && ($node->tnid != $node->nid))) {
    $form['simplenews']['#description'] = t('This newsletter issue is part of a translation set. Sending this set is controlled from the <a href="@link">translation source newsletter</a>.', array('@link' => url('node/' . $node->tnid)));
    // @todo Translated nodes must also have the same Category!
    // Move the category in here and give the user feedback.
  }
  else {
    // Show newsletter sending options if newsletter has not been sent yet.
    // If send a notification is shown.
    if ($simplenews_values->status == SIMPLENEWS_STATUS_SEND_NOT || $simplenews_values->status == SIMPLENEWS_STATUS_SEND_PUBLISH) {

      $options = array(
        SIMPLENEWS_COMMAND_SEND_TEST => t('Send one test newsletter to the test address'),
      );

      // Add option to send on publish when the node is unpublished.
      if ($node->status == NODE_NOT_PUBLISHED) {
        $options[SIMPLENEWS_COMMAND_SEND_PUBLISH] = t('Send newsletter when published');
      }
      else {
        $options[SIMPLENEWS_COMMAND_SEND_NOW] = t('Send newsletter');
      }

      if ($simplenews_values->status == SIMPLENEWS_STATUS_SEND_PUBLISH) {
        $send_default = SIMPLENEWS_STATUS_SEND_PUBLISH;
      }
      else {
        $send_default = $config->get('send');
      }
      $form['simplenews']['send'] = array(
        '#type' => 'radios',
        '#title' => t('Send newsletter'),
        '#default_value' => $simplenews_values->send ?? $send_default,
        '#options' => $options,
        '#attributes' => array(
          'class' => array('simplenews-command-send'),
        ),
      );

      $address_default = config_get('system.core', 'site_mail');
      if ($config->get('test_address_override')) {
        $form['simplenews']['test_address'] = array(
          '#type' => 'textfield',
          '#title' => t('Test email addresses'),
          '#description' => t('A comma-separated list of email addresses to be used as test addresses.'),
          '#default_value' => $config->get('test_address'),
          '#size' => 60,
          '#maxlength' => 128,
        );
      }
      else {
        $form['simplenews']['test_address'] = array(
          '#type' => 'value',
          '#value' => $config->get('test_address'),
        );
      }
    }
    else {
      $form['simplenews']['none'] = array(
        '#type' => 'checkbox',
        '#return_value' => 0,
        '#attributes' => array(
          'checked' => 'checked',
          'disabled' => 'disabled',
        ),
      );
      $form['simplenews']['none']['#title'] = ($simplenews_values->status == SIMPLENEWS_STATUS_SEND_READY) ? t('This newsletter has been sent') : t('This newsletter is pending');
      return $form;
    }
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Send'),
    );
  }
  return $form;
}

/**
 * @todo
 */
function simplenews_node_tab_send_form_validate($form, &$form_state) {
  $config = config('simplenews.settings');
  $values = $form_state['values'];
  $node = node_load($values['nid']);

  $default_address = $config->get('test_address');
  $mails = array($default_address);
  if (isset($values['simplenews']['send']) && $values['simplenews']['send'] == SIMPLENEWS_COMMAND_SEND_TEST && $config->get('test_address_override')) {
    // @todo Can we simplify and use only two kind of messages?
    if (!empty($values['simplenews']['test_address'])) {
      $mails = explode(',', $values['simplenews']['test_address']);
      foreach ($mails as $mail) {
        $mail = trim($mail);
        if ($mail == '') {
          form_set_error('simplenews][test_address', t('Test email address is empty.'));
        }
        elseif (!valid_email_address($mail)) {
          form_set_error('simplenews][test_address', t('Invalid email address "%mail".', array('%mail' => $mail)));
        }
      }
    }
    else {
      form_set_error('simplenews][test_address', t('Missing test email address.'));
    }
  }
  $form_state['test_addresses'] = $mails;
}

/**
 * @todo
 */
function simplenews_node_tab_send_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $node = node_load($values['nid']);

  // Send newsletter to all subscribers or send test newsletter.
  module_load_include('inc', 'simplenews', 'includes/simplenews.mail');
  if ($values['simplenews']['send'] == SIMPLENEWS_COMMAND_SEND_NOW) {
    simplenews_add_node_to_spool($node);
    // Attempt to send immediately, if configured to do so.
    if (simplenews_mail_attempt_immediate_send(array('nid' => $node->nid))) {
      backdrop_set_message(t('Newsletter %title sent.', array('%title' => $node->title)));
    }
    else {
      backdrop_set_message(t('Newsletter %title pending.', array('%title' => $node->title)));
    }
  }
  elseif ($values['simplenews']['send'] == SIMPLENEWS_COMMAND_SEND_TEST) {
    simplenews_send_test($node, $form_state['test_addresses']);
  }

  // If the selected command is "Send on publish", just set the newsletter status.
  if ($values['simplenews']['send'] == SIMPLENEWS_COMMAND_SEND_PUBLISH) {
    $newsletter = simplenews_newsletter_load($node->nid);
    if (!$newsletter) {
      $newsletter = simplenews_newsletter_defaults($node);
    }
    $newsletter->status = SIMPLENEWS_STATUS_SEND_PUBLISH;
    simplenews_newsletter_save($newsletter);
    backdrop_set_message(t('The newsletter will be sent when the content is published.'));
  }
  // Clear the node cache for this node so that the status shows correctly when
  // loaded.
  cache_clear_all($node->nid, 'cache_entity_node');
}
