<?php

function secsyn_node_syndicate_form($form, &$state, $node) {
  drupal_set_title(t('Syndicate !title', array('!title' => $node->title)));
  $form['remote_server'] = array(
    '#title' => t('Syndicate With'),
    '#type' => 'select',
    '#options' => twoway_server_list(),
    '#required' => TRUE,
    '#default' => array(),
    '#empty_value' => '',
  );
  if (user_access('urgent syndication')) {
    $form['urgency'] = array(
      '#type' => 'checkbox',
      '#title' => t('Urgent: Push ASAP'),
    );
  }
  $form['nid'] = array(
    '#type' => 'value',
    '#value' => $node->nid,
  );
  $form['submit'] = array(
    '#value' => t('Syndicate'),
    '#type' => 'submit'
  );
  return $form;
}

function secsyn_node_syndicate_form_submit($form, &$form_state) {
  $node = node_load($form_state['values']['nid']);
  $remote_server = twoway_server_load($form_state['values']['remote_server']);
  $engine = new SecureSyndicationEngine();
  $urgent = FALSE;
  if (!empty($form_state['values']['urgency'])) {
    $urgent = TRUE;
  }
  if ($engine->queue($remote_server['server_connection'], 'SecureSyndicationEntityProfile', $node)) {
    drupal_set_message(t('Content has been queued for syndication'));
  }
  $form_state['redirect'] = 'node/' . $form_state['values']['nid'];
}