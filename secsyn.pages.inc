<?php

function secsyn_force_page() {
  module_load_include('inc', 'secsyn', 'secsyn.functions');
  secsyn_syndicate_all(FALSE);
  return '<p>' . t('Syndication complete') . '</p>';
}

function secsyn_status_page() {
  $engine = new SecureSyndicationEngine();
  $content = '<p>';
  $content .= '<strong>' . t('Waiting for syndication: @total', array('@total' => $engine->waitingQueueSize())) . '</strong><br />';
  $content .= '<strong>' . t('Queued for validation: @total', array('@total' => $engine->validationQueueSize())) . '</strong><br />';
  $content .= '<strong>' . t('Queued for syndication: @total', array('@total' => $engine->syndicationQueueSize())) . '</strong><br />';
  $content .= '</p>';
  return $content;
}