<?php

function secsyn_bean_syndicate_form($form, &$form_state, $object = NULL) {
  $bean = bean_load_delta($object);
  $form['bean'] = array(
    '#value' => $bean,
    '#type' => 'value',
  );
  _secsyn_common_fields($form);
  $form['submit'] = array(
    '#value' => t('Syndicate'),
    '#type' => 'submit'
  );
  return $form;
}

function secsyn_bean_syndicate_form_submit($form, &$form_state) {
  if (_secsyn_fast_queue(
      $form_state['values']['remote_server'],
      'SecureSyndicationEntityProfile',
      $form_state['values']['bean'],
      !empty($form_state['values']['urgency'])
  )) {
    drupal_set_message(t('Bean queued for syndication'));
  }
  $form_state['redirect'] = 'block/' . $form_state['values']['bean']->delta;
}

